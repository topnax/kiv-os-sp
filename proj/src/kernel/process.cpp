//
// Created by Stanislav Král on 11.10.2020.
//
#include <thread>
#include "../api/hal.h"
#include "../api/api.h"
#include <map>
#include <deque>
#include <random>
#include "handles.h"
#include "semaphore.h"
#include "process.h"

std::mutex Semaphores_Mutex;
std::map<std::thread::id, kiv_os::THandle> Handle_To_THandle;
std::map<kiv_os::THandle, std::deque<Semaphore*>*> Thread_Semaphores;

int Last_Semaphore_ID = 0;
std::random_device rd_s;
std::mt19937 gen_s(rd_s());
std::uniform_int_distribution<> dis_s(1, 6);


void Handle_Process(kiv_hal::TRegisters &regs) {
    switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

        case kiv_os::NOS_Process::Clone: {
            clone(regs);
        }
            break;

        case kiv_os::NOS_Process::Wait_For: {
            wait_for(regs);
        }
        break;
    }
}

void thread_entrypoint(kiv_hal::TRegisters &registers) {
    // execute the TThread_Proc (the desired program, echo, md, ...)
    ((kiv_os::TThread_Proc) registers.rdx.r)(registers);

    // the program is now executed

    // while cloning, the THandle is generated and assigned to the Handle_To_THandle map
    // under the key which is the thread ID of the thread the TThread_Proc is run in

    // get the kiv_os::THandle
    kiv_os::THandle t_handle = Handle_To_THandle[std::this_thread::get_id()];

    // get all semaphores that are waiting for this thread
    auto resolved = Thread_Semaphores.find(t_handle);

    if (resolved != Thread_Semaphores.end()) {
        // found semaphore list
        std::deque<Semaphore *> *semaphores = Thread_Semaphores[t_handle];
        // iterate over thread's semaphores
        for (Semaphore *se : *semaphores) {
            // notify the semaphores of the thread
            se->notify();
        }
    }

    // remove the THandle
    Remove_Handle(t_handle);

    // remove the std::thread::id to kiv_os::THandle mapping
    Handle_To_THandle.erase(std::this_thread::get_id());
}

void clone(kiv_hal::TRegisters &registers) {
    // spawn a new thread, in which a program at address inside rdx is run
    std::thread t1(thread_entrypoint, registers);

    // get the native handle of the spawned thread
    HANDLE native_handle = t1.native_handle();

    // generate a kiv_os::THandle based on the native handle
    kiv_os::THandle t_handle = Convert_Native_Handle(native_handle);

    // TODO is it really necessary to map the thread ID to the THandle?

    // map the ID of the spawned thread to the native handle
    Handle_To_THandle[t1.get_id()] = t_handle;

    // based on the specification of the Clone syscall, save the generated THandle to the rax register
    registers.rax.x = t_handle;

    // detach the spawned thread - joining is done using semaphores
    t1.detach();
}

void wait_for(kiv_hal::TRegisters &registers) {
    // cast rdx to THandle array
    auto *t_handles = reinterpret_cast<kiv_os::THandle *>(registers.rdx.r);

    // load handle count from rcx
    uint8_t handleCount = registers.rcx.l;

    // TODO is it really necessary to assign the semaphore an unique ID?

    // create a semaphore with an ID so we are able to delete it later
    Last_Semaphore_ID += dis_s(gen_s);
    Semaphore s(0, Last_Semaphore_ID);

    Semaphores_Mutex.lock();
    int index = 0;
    bool anyHandleInvalid = false;

    // try to add the semaphore for each thread handle passed in the handles array
    for (index = 0; index < handleCount; index++) {
        // load a handle from the array
        auto t_handle = t_handles[index];

        // current thread has no list of semaphores - let's create one
        if (!Thread_Semaphores[t_handle]) {
            auto dq = new std::deque<Semaphore *>;
            Thread_Semaphores[t_handle] = dq;
        }

        // check whether the handle is valid
        if (Resolve_kiv_os_Handle(t_handle) != INVALID_HANDLE_VALUE) {
            // assign a semaphore to the THandle
            (*(Thread_Semaphores[t_handle])).push_back(&s);
        } else {
            // stop iterating over handles - we found a handle that is not valid
            anyHandleInvalid = true;
            break;
        }
    }

    // an invalid handle was found, remove all semaphores (rollback)
    if (anyHandleInvalid) {
        // iterate to the last index we have added a semaphore to
        for (int i = 0; i < index; i++) {
            // load a handle
            auto t_handle = t_handles[i];

            // TODO we might use the ID of the semaphore to remove it

            std::deque<Semaphore *> *sem_list = Thread_Semaphores[t_handle];
            // pop back one semaphore
            // this is the reason why we use a mutex to make sure the semaphore deque does not change while we loop over handles
            if (sem_list) {
                sem_list->pop_back();
                // if the semaphore list is empty, delete it from the thread to semaphores map
                if (sem_list->empty()) {
                    Thread_Semaphores.erase(t_handle);
                }
            }
        }

        // finally, unlock the mutex, finish the syscall
        Semaphores_Mutex.unlock();

        // set the rax register to handleCount (error result - our own convention, not specified in the API)
        registers.rax.l = handleCount;
    } else {
        // no invalid handle found - unlock the mutex
        Semaphores_Mutex.unlock();

        // wait for any thread to notify the semaphore
        s.wait();

        // find the index of the handle that has signalled the semaphore
        uint8_t handleThatSignalledIndex;

        // semaphore notified - remove all semaphores from each handle semaphore deque
        for (uint8_t i = 0; i < handleCount; i++) {
            // load a handle
            auto t_handle = t_handles[i];

            if (Resolve_kiv_os_Handle(t_handle) == INVALID_HANDLE_VALUE) {
                // cannot resolve THandle to native_handle - this is the handle that signalled the semaphore
                // because the handle is removed once the thread has finished
                handleThatSignalledIndex = i;
            }

            // iterate over semaphores of the currently iterated handle
            auto current_handle_semaphores = Thread_Semaphores[t_handle];

            int index_to_be_removed = 0;
            for (Semaphore *sem : *current_handle_semaphores) {
                // erase the semaphore that we have added
                if (sem->get_id() == s.get_id()) {
                    break;
                }
                index_to_be_removed++;
            }

            current_handle_semaphores->erase(current_handle_semaphores->begin() + index_to_be_removed);

            // TODO should we delete (remove from heap) the created semaphore?
        }

        // save the index of the handle that has signalled the semaphore to the rax register according to the API specification
        registers.rax.l = handleThatSignalledIndex;
    }
}
