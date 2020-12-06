//
// Created by Stanislav Král on 11.10.2020.
//
#include <memory>
#include <thread>
#include "../api/hal.h"
#include "../api/api.h"
#include <map>
#include <random>
#include "handles.h"
#include "semaphore.h"
#include "process.h"
#include "files.h"


namespace Proc {
    Thread_Control_Block *Tcb = new Thread_Control_Block();
    Process_Control_Block *Pcb = new Process_Control_Block();
    kiv_os::THandle Last_Listener_ID = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 6);
}

// TODO move to Process namespace

// std::mutex Semaphores_Mutex;
std::mutex Semaphores_Mutex;
std::map<std::thread::id, kiv_os::THandle> Handle_To_THandle;

Process_Control_Block *Get_Pcb() {
    return Proc::Pcb;
}

Thread_Control_Block *Get_Tcb() {
    return Proc::Tcb;
}

size_t __stdcall default_signal_handler(const kiv_hal::TRegisters &regs) {
    auto signal_id = static_cast<kiv_os::NSignal_Id>(regs.rcx.l);
    // process the signal
    return 0;
}

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

        case kiv_os::NOS_Process::Register_Signal_Handler: {
            register_signal_handler(regs);
        }
            break;

        case kiv_os::NOS_Process::Shutdown: {
            shutdown();
        }
            break;
    }
}

kiv_hal::TRegisters Prepare_Process_Context(unsigned short std_in, unsigned short std_out, char *args) {
    kiv_hal::TRegisters regs{};
    regs.rax.x = std_in;
    regs.rbx.x = std_out;
    regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(args);
    return regs;
}

void thread_entrypoint(kiv_os::TThread_Proc t_threadproc, kiv_hal::TRegisters &registers, bool is_process, Semaphore *s) {
    // do not execute any code until the passed semaphore is notified
    s->wait();

    // delete the passed semaphore
    delete s;

    // execute the TThread_Proc (the desired program, echo, md, ...)
    t_threadproc(registers);

    // the program is now executed - call thread post execute
    thread_post_execute(is_process);
}

void thread_post_execute(bool is_process){
    // while cloning, the THandle is generated and assigned to the Handle_To_THandle map
    // under the key which is the thread ID of the thread the TThread_Proc is run in
    std::lock_guard<std::mutex> guard(Semaphores_Mutex);

    // get the kiv_os::THandle
    kiv_os::THandle t_handle = Handle_To_THandle[std::this_thread::get_id()];

    if (is_process) {
        // set the status of the process that has just ended to Zombie (the process is waiting for someone to read it's exit code)
        auto process = (*Proc::Pcb)[t_handle];
        if (process != nullptr) {
            process->status = Process_Status::Zombie;
        }
    }

    // get all listeners that are waiting for this thread
    auto resolved = Proc::Tcb->Wait_For_Listeners.find(t_handle);


    // remove the THandle before notifying listeners
    Remove_Handle(t_handle);

    if (resolved != Proc::Tcb->Wait_For_Listeners.end()) {
        // found a listener list
        auto *listeners = Proc::Tcb->Wait_For_Listeners[t_handle].get();
        // iterate over thread's listeners
        for (auto *listener : *listeners) {
            // notify the listeners of the thread
            listener->handle_that_notified = t_handle;
            listener->notified = true;
            listener->semaphore->notify();
        }

        // after all listeners have been notified, we can clear the list
        listeners->clear();

        // erase the list from the map
        Proc::Tcb->Wait_For_Listeners.erase(t_handle);
    }

    if (Proc::Tcb->Parent_Processes.find(t_handle) != Proc::Tcb->Parent_Processes.end()) {
        // remove the reference to the thread's parent process from the map
        Proc::Tcb->Parent_Processes.erase(t_handle);
    }

    // remove the std::thread::id to kiv_os::THandle mapping
    Handle_To_THandle.erase(std::this_thread::get_id());
}

void run_in_a_thread(kiv_os::TThread_Proc t_threadproc, kiv_hal::TRegisters &registers, bool is_process, Semaphore *s) {
    // we are adding to the thread handle to Handle_To_THandle, better guard this block
    // TODO this might not be necessary
    std::lock_guard<std::mutex> guard(Semaphores_Mutex);

    // spawn a new thread, in which a program at address inside rdx is run
    std::thread t1(thread_entrypoint, t_threadproc, registers, is_process, s);

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
            // create a semaphore that will be notified once we have gathered necessary information about the spawned thread
            auto s = new Semaphore(0);

            kiv_hal::TRegisters regs{};

            // copy thread arguments
            regs.rdi.r = registers.rdi.r;

            // run the TThreadProc in a new thread
            run_in_a_thread(((kiv_os::TThread_Proc) registers.rdx.r), regs, false, s);

            // get the kiv_os::THandle
            // resolve the handle of the current thread
            auto resolved = Handle_To_THandle.find(std::this_thread::get_id());

            // get the handle of the spawned thread
            kiv_os::THandle thread_handle = regs.rax.x;

            // check whether we resolved the process handle of the current thread
            if (resolved != Handle_To_THandle.end()) {
                auto resolved_process = Get_Pcb()->operator[](resolved->second);
                if (resolved_process != nullptr) {
                    // current thread handle is resolved to a process
                    (*Proc::Tcb).Parent_Processes[thread_handle] = resolved_process->handle;
                }
            }

            registers.rax.x = thread_handle;
            registers.flags.carry = 0;

            s->notify();
        }
        break;

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
                // create a new semaphore that will be signaled once this thread gathers information about the created process
                auto s = new Semaphore(0);

                // pass a pointer to the semaphore to the thread to be created
                // the thread will not execute any instructions until this semaphore is notified
                run_in_a_thread(program_entrypoint, regs, true, s);

                // get the handle of the spawned thread
                auto process = Proc::Pcb->Add_Process(regs.rax.x, std_in, std_out, program);
                process->status = Process_Status::Running;
                process->signal_handlers[kiv_os::NSignal_Id::Terminate] = default_signal_handler;

                auto parent_process = resolve_current_thread_handle_to_process();
                if (parent_process != nullptr) {
                    process->working_directory = parent_process->working_directory;
                }
                registers.rax.x = regs.rax.x;

                // we have gathered all necessary information about the spawned thread and have created a PCB entry based on it
                s->notify();
            } else {
                registers.flags.carry = 1;
                registers.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
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

    std::unique_lock<std::mutex> lock(Semaphores_Mutex);

    // generate an unique ID for the new listener
    Proc::Last_Listener_ID += Proc::dis(Proc::gen);
    Wait_For_Listener current_listener {
        Proc::Last_Listener_ID,
        std::make_unique<Semaphore>(0),
        0,
        false
    };

    int index;
    bool anyHandleInvalid = false;

    // try to add the listener for each thread handle passed in the handles array
    for (index = 0; index < handleCount; index++) {
        // load a handle from the array
        auto t_handle = t_handles[index];

        // check whether the handle is valid
        if (Resolve_kiv_os_Handle(t_handle) != INVALID_HANDLE_VALUE) {
            // current thread has no list of listeners - let's create one
            if (!Proc::Tcb->Wait_For_Listeners[t_handle]) {
                Proc::Tcb->Wait_For_Listeners[t_handle] = std::make_unique<std::vector<Wait_For_Listener *>>();
            }
            // assign a listener to the THandle
            (*(Proc::Tcb->Wait_For_Listeners[t_handle])).push_back(&current_listener);
        } else {
            // stop iterating over handles - we found a handle that is not valid
            anyHandleInvalid = true;
            break;
        }
    }

    // an invalid handle was found, remove all listeners (rollback)
    if (anyHandleInvalid) {
        // iterate to the last index we have added the listener to
        // TODO might be <= index???
        for (int i = 0; i < index; i++) {
            // load a handle
            auto t_handle = t_handles[i];

            std::vector<Wait_For_Listener *> *wait_for_listeners = Proc::Tcb->Wait_For_Listeners[t_handle].get();
            // pop back one listener
            // this is the reason why we use a mutex to make sure the listener vector does not change while we loop over handles
            if (wait_for_listeners) {
                wait_for_listeners->pop_back();
                // if the listener list is empty, delete it from the thread to listeners map
                if (wait_for_listeners->empty()) {
                    Proc::Tcb->Wait_For_Listeners.erase(t_handle);
                }
            }
        }

        auto invalid_handle = t_handles[index];
        // check whether the invalid handle has a record in the PCB table
        if (Proc::Pcb->operator [](invalid_handle) != nullptr) {
            // if yes, then return the invalid handle index
            registers.rax.l = index;
        } else {
            // set the rax register to handleCount (error result - our own convention, not specified in the API)
            registers.rax.l = handleCount;
        }

        // finally, unlock the mutex, finish the syscall
        lock.unlock();
    } else {
        // no invalid handle found - unlock the mutex
        lock.unlock();

        // wait for any thread to notify this semaphore
        current_listener.semaphore->wait();

        // find the index of the handle that has signalled the listener
        uint8_t handleThatSignalledIndex = -1; // initialize the variable otherwise problems upon calling exit?

        if (!current_listener.notified) {
            handleThatSignalledIndex = handleCount;
        } else {
            // we are modifying the listener vectors - guard following code using mutex
            std::lock_guard<std::mutex> guard(Semaphores_Mutex);
            for (uint8_t i = 0; i < handleCount; i++) {
                // load a handle
                auto t_handle = t_handles[i];
                if (t_handle == current_listener.handle_that_notified) {
                    handleThatSignalledIndex = i;
                    // no need to remove this listener - all listeners of the thread that has notified this listener
                    // are removed in the thread_post_execute function
                } else {
                    auto resolved = Proc::Tcb->Wait_For_Listeners.find(t_handle);
                    if (resolved != Proc::Tcb->Wait_For_Listeners.end()) {
                        // remove the created listener from all threads we wanted to wait for if still present in the listener map
                        auto listeners = resolved->second.get();

                        // iterate over listeners of the current handle
                        for (size_t j = 0; j < listeners->size(); j++) {
                            auto listener = listeners->at(j);

                            // check whether this is the listener we have created
                            if (listener->id == current_listener.id) {
                                // if yes, then remove it from the list of the
                                // acquire semaphore mutex before removing it
                                std::lock_guard<std::mutex> l(listener->semaphore->mtx);
                                listeners->erase(listeners->begin() + j);
                                break;
                            }
                        }

                        // remove the vector from the map if it's empty
                        if (listeners->empty()) {
                            Proc::Tcb->Wait_For_Listeners.erase(t_handle);
                        }
                    }
                }
            }
        }

        // save the index of the handle that has notified the listener to the rax register according to the API specification
        registers.rax.l = handleThatSignalledIndex;
    }
}

void exit(kiv_os::THandle handle, kiv_os::NOS_Error exit_code) {
    // store the exit code into the PCB under the key of the handle
    auto process = (*Proc::Pcb)[handle];
    if (process != nullptr) {
        // set the exit code
        process->exit_code = exit_code;
        // set the status to Zombie
        process->status = Process_Status::Zombie;
    }
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
        kiv_hal::TRegisters regs{};

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
    auto process = (*Proc::Pcb)[handle];

    std::lock_guard<std::mutex> guard(Semaphores_Mutex);
    if (process != nullptr && process->status == Process_Status::Zombie) {
        exit_code = process->exit_code;

        // we have read the process' exit code - remove it from the table
        (*Proc::Pcb).Remove_Process(handle);
    } else {
        // TODO in this case EC is set to Unknown_Error, but what should we do, if the exit code has already been read?
    }

    registers.rcx.x = static_cast<decltype(registers.rcx.x)>(exit_code);
}

void register_signal_handler(kiv_hal::TRegisters &registers) {
    auto signal_id = static_cast<kiv_os::NSignal_Id>(registers.rcx.l);
    auto thread_proc = reinterpret_cast<kiv_os::TThread_Proc>(registers.rdx.r);

    // find the handle of the current thread
    auto resolved = Handle_To_THandle.find(std::this_thread::get_id());
    if (resolved != Handle_To_THandle.end()) {
        auto process = Proc::Pcb->operator[](resolved->second);
        // check whether the current handle is a process (has an entry in the PCB table)
        if (process != nullptr) {
            // register the given thread_proc in case of non-zero thread_proc passed via registers
            process->signal_handlers[signal_id] = thread_proc ? thread_proc : default_signal_handler;
        }
    }
}

void signal_all_processes(kiv_os::NSignal_Id signal_id) {
    // iterate over all processes and signal the given signal
    for (Process *process : Proc::Pcb->Get_Processes()) {
        signal(signal_id, process);
    }
}

void signal(kiv_os::NSignal_Id signal_id, Process *process) {
    auto resolved = process->signal_handlers.find(signal_id);
    if (resolved != process->signal_handlers.end()) {
        kiv_hal::TRegisters regs{};
        regs.rcx.l = static_cast<decltype(regs.rcx.l)>(signal_id);
        // execute the handler of the given signal for the given process
        resolved->second(regs);
    }
}

void shutdown() {
    // iterate over all processes and write EOT to their STD_IN

    signal_all_processes(kiv_os::NSignal_Id::Terminate);

    for (Process *process : Proc::Pcb->Get_Processes()) {
        kiv_hal::TRegisters regs{};
        regs.rdx.x = static_cast<decltype(regs.rdx.x)>(process->std_in);
        Close_File(regs);
    }
}

bool resolve_native_thread_id_to_thandle(std::thread::id native_thread_id, kiv_os::THandle &out_handle) {
    auto resolved = Handle_To_THandle.find(native_thread_id);
    if (resolved != Handle_To_THandle.end()) {
        out_handle = resolved->second;
        return true;
    }
    return false;
}

Process *resolve_current_thread_handle_to_process() {
    // resolve the handle of the current native thread
    kiv_os::THandle thread_handle;
    bool thread_handle_resolved = resolve_native_thread_id_to_thandle(std::this_thread::get_id(), thread_handle);

    Process *process = nullptr;
    if (thread_handle_resolved) {
        // native thread has been mapped to THandle

        auto pcb = Get_Pcb();

        // check whether the current native thread is a kiv_os process
        auto p = pcb->operator[](thread_handle);
        if (p != nullptr) {
            // it really is a process
            process = p;
        }  else {
            // it might be kiv_os thread, check whether the thread has a parent pid assigned
            auto tcb = Get_Tcb();
            auto resolved_thread_thandle = tcb->Parent_Processes.find(thread_handle);
            if (resolved_thread_thandle != tcb->Parent_Processes.end()) {
                // current kiv_os thread has a pid assigned, find the process using the pid
                process = pcb->operator[](resolved_thread_thandle->second);
            }
        }

    }
    return process;
}
