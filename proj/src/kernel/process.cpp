//
// Created by Stanislav Král on 11.10.2020.
//
#include <memory>
#include <thread>
#include "../api/hal.h"
#include "../api/api.h"
#include <map>
#include <deque>
#include <random>
#include "handles.h"
#include "semaphore.h"
#include "process.h"

class Semaphores {
public:
    std::map<kiv_os::THandle, std::unique_ptr<std::vector<Semaphore *>>> Thread_Semaphores;
};

namespace Process {
    Semaphores *semaphores = new Semaphores();
}

// TODO move to Process namespace

std::mutex Semaphores_Mutex;
std::map<std::thread::id, kiv_os::THandle> Handle_To_THandle;
std::map<kiv_os::THandle, kiv_os::NOS_Error> Pcb;

int Last_Semaphore_ID = 0;
std::random_device rd_s;
std::mt19937 gen_s(rd_s());
std::uniform_int_distribution<> dis_s(1, 6);


void Handle_Process(kiv_hal::TRegisters &regs, HMODULE user_programs) {
    switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

        case kiv_os::NOS_Process::Clone: {
            clone(regs, user_programs);
        }
            break;

        case kiv_os::NOS_Process::Wait_For: {
            wait_for(regs);
        }
            break;

        case kiv_os::NOS_Process::Exit: {
            exit(regs);
        }
            break;

        case kiv_os::NOS_Process::Read_Exit_Code: {
            read_exit_code(regs);
        }
            break;
    }
}

kiv_hal::TRegisters Prepare_Process_Context(unsigned short std_in, unsigned short std_out, char *args) {
    kiv_hal::TRegisters regs;
    regs.rax.x = std_in;
    regs.rbx.x = std_out;
    regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(args);
    return regs;
}

void thread_entrypoint(kiv_os::TThread_Proc t_threadproc, kiv_hal::TRegisters &registers, bool is_process) {
    // execute the TThread_Proc (the desired program, echo, md, ...)
    t_threadproc(registers);

    // the program is now executed - call thread post execute
    thread_post_execute(is_process);
}

void thread_post_execute(bool is_process) {// while cloning, the THandle is generated and assigned to the Handle_To_THandle map
// under the key which is the thread ID of the thread the TThread_Proc is run in

    // get the kiv_os::THandle
    kiv_os::THandle t_handle = Handle_To_THandle[std::this_thread::get_id()];

    if (is_process) {
        // if no exit code is found in the PCB table for the current THandle, implicitly set it to Success
        if (Pcb.find(t_handle) == Pcb.end()) {
            exit(t_handle, kiv_os::NOS_Error::Success);
        }
    }

    // get all semaphores that are waiting for this thread
    auto resolved = Process::semaphores->Thread_Semaphores.find(t_handle);


    // remove the THandle before notifying semaphores
    Remove_Handle(t_handle);

    if (resolved != Process::semaphores->Thread_Semaphores.end()) {
        // found semaphore list
        auto *semaphores = Process::semaphores->Thread_Semaphores[t_handle].get();
        // iterate over thread's semaphores
        for (Semaphore *se : *semaphores) {
            // notify the semaphores of the thread
            se->notify();
        }
    }

    // remove the std::thread::id to kiv_os::THandle mapping
    Handle_To_THandle.erase(std::this_thread::get_id());
}

void run_in_a_thread(kiv_os::TThread_Proc t_threadproc, kiv_hal::TRegisters &registers, bool is_process) {
    // spawn a new thread, in which a program at address inside rdx is run
    std::thread t1(thread_entrypoint, t_threadproc, registers, is_process);

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

void clone(kiv_hal::TRegisters &registers, HMODULE user_programs) {
    switch (static_cast<kiv_os::NClone>(registers.rcx.l)) {
        case kiv_os::NClone::Create_Thread: {
            // run the TThreadProc in a new thread
            run_in_a_thread(((kiv_os::TThread_Proc) registers.rdx.r), registers, false);
            break;
        }

        case kiv_os::NClone::Create_Process: {
            // load program name from arguments
            char *program = (char *) registers.rdx.r;

            // load program arguments
            char *arguments = (char *) registers.rdi.r;

            // load stdin
            kiv_os::THandle std_in = (registers.rbx.e >> 16) & 0xFFFF;
            // load stdout
            kiv_os::THandle std_out = registers.rbx.e & 0xFFFF;

            kiv_hal::TRegisters regs = Prepare_Process_Context(std_in, std_out, arguments);
            kiv_os::TThread_Proc program_entrypoint = (kiv_os::TThread_Proc) GetProcAddress(user_programs, program);
            if (program_entrypoint) {
                run_in_a_thread(program_entrypoint, regs, true);
                registers.rax.x = regs.rax.x;
            }

            break;
        }
    }
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
        if (!Process::semaphores->Thread_Semaphores[t_handle]) {
            Process::semaphores->Thread_Semaphores[t_handle] = std::make_unique<std::vector<Semaphore *>>();
        }

        // check whether the handle is valid
        if (Resolve_kiv_os_Handle(t_handle) != INVALID_HANDLE_VALUE) {
            // assign a semaphore to the THandle
            (*(Process::semaphores->Thread_Semaphores[t_handle])).push_back(&s);
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

            std::vector<Semaphore *> *sem_list = Process::semaphores->Thread_Semaphores[t_handle].get();
            // pop back one semaphore
            // this is the reason why we use a mutex to make sure the semaphore deque does not change while we loop over handles
            if (sem_list) {
                sem_list->pop_back();
                // if the semaphore list is empty, delete it from the thread to semaphores map
                if (sem_list->empty()) {
                    Process::semaphores->Thread_Semaphores.erase(t_handle);
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
            auto current_handle_semaphores = Process::semaphores->Thread_Semaphores[t_handle].get();

            int index_to_be_removed = 0;
            for (Semaphore *sem : *current_handle_semaphores) {
                // erase the semaphore that we have added
                if (sem->get_id() == s.get_id()) {
                    break;
                }
                index_to_be_removed++;
            }
            auto se = current_handle_semaphores->erase(current_handle_semaphores->begin() + index_to_be_removed);
            if (current_handle_semaphores->empty()) {
                Process::semaphores->Thread_Semaphores.erase(t_handle);
            }
        }
        // save the index of the handle that has signalled the semaphore to the rax register according to the API specification
        registers.rax.l = handleThatSignalledIndex;
    }
}

void exit(kiv_os::THandle handle, kiv_os::NOS_Error exit_code) {
    // store the exit code into the PCB under the key of the handle
    Pcb[handle] = exit_code;
}

void exit(kiv_hal::TRegisters &registers) {
    auto exit_code = static_cast<kiv_os::NOS_Error>(registers.rcx.x);

    // find the handle of the current thread
    auto resolved = Handle_To_THandle.find(std::this_thread::get_id());
    if (resolved != Handle_To_THandle.end()) {
        // exit with the given exit code
        exit(resolved->second, exit_code);
    }

    // TODO somehow terminate the thread? std::thread cannot be terminated however
}

void read_exit_code(kiv_hal::TRegisters &registers) {
    kiv_os::THandle handle = registers.rdx.x;

    // check whether the handle is still referencing an active thread
    if (Resolve_kiv_os_Handle(handle) != INVALID_HANDLE_VALUE) {
        kiv_hal::TRegisters regs;

        kiv_os::THandle handles[] = {handle};

        // save pointer to the THandle to registers
        regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles);

        // store the count of handles to registers
        regs.rcx.l = 1;

        // wait for the handle to finish
        wait_for(regs);
    }

    kiv_os::NOS_Error exit_code = kiv_os::NOS_Error::Unknown_Error;

    // the handle has finished now, find the exit code
    auto resolved = Pcb.find(handle);
    if (resolved != Pcb.end()) {
        exit_code = resolved->second;
        Pcb.erase(handle);
    } else {
        // TODO in this case EC is set to Unknown_Error, but what should we do, if the exit code has already been read?
    }

    registers.rcx.x = static_cast<decltype(registers.rcx.x)>(exit_code);
}
