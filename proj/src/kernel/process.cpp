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
#include "process/pcb.h"


namespace Proc {
    Thread_Control_Block *Tcb = new Thread_Control_Block();
    Process_Control_Block *Pcb = new Process_Control_Block();
    kiv_os::THandle Last_Listener_ID = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 6);
}

// TODO move to Process namespace

std::mutex Semaphores_Mutex;
std::map<std::thread::id, kiv_os::THandle> Handle_To_THandle;
std::map<kiv_os::THandle, kiv_os::NOS_Error> Pcb;


size_t __stdcall default_signal_handler(const kiv_hal::TRegisters &regs) {
    auto signal_id = static_cast<kiv_os::NSignal_Id>(regs.rcx.l);
    auto resolved = Handle_To_THandle.find(std::this_thread::get_id());

    if (resolved != Handle_To_THandle.end()) {
        auto process = Proc::Pcb->operator[](resolved->second);
        if (process != nullptr) {
            // TODO remove for release
            printf("Default handler of signal=%hhu for process=%d executed...", signal_id, process->handle);
        }
    }
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
    std::lock_guard<std::mutex> guard(Semaphores_Mutex);

    // get the kiv_os::THandle
    kiv_os::THandle t_handle = Handle_To_THandle[std::this_thread::get_id()];

    if (is_process) {
        // set the status of the process that has just ended to Zombie (the process is waiting for someone to read it's exit code)
        auto process = (*Proc::Pcb)[t_handle];
        if (process != nullptr) {
            process->status = Process_Status::Zombie;
            // process->signal_handlers.clear();
        } else {
            printf("could not find process with handle of %d\n", t_handle);
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

    // remove the std::thread::id to kiv_os::THandle mapping
    Handle_To_THandle.erase(std::this_thread::get_id());
}

void run_in_a_thread(kiv_os::TThread_Proc t_threadproc, kiv_hal::TRegisters &registers, bool is_process) {
    // we are adding to the thread handle to Handle_To_THandle, better guard this block
    // TODO this might not be necessary
    std::lock_guard<std::mutex> guard(Semaphores_Mutex);

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
                auto process = Proc::Pcb->Add_Process(regs.rax.x, std_in, std_out, program);
                process->status = Process_Status::Running;
                process->signal_handlers[kiv_os::NSignal_Id::Terminate] = default_signal_handler;
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

    std::unique_lock<std::mutex> lock(Semaphores_Mutex);

    // generate an unique ID for the new listener
    Proc::Last_Listener_ID += Proc::dis(Proc::gen);
    Wait_For_Listener current_listener {
        Proc::Last_Listener_ID,
        std::make_unique<Semaphore>(0),
        0,
        false
    };

    int index = 0;
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
        if (Pcb.find(invalid_handle) != Pcb.end()) {
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
            // TODO remove for release
            printf("semaphore not notified!!!\n");
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
                    // remove the created listener from all threads we wanted to wait for
                    auto listeners = Process::Tcb->Wait_For_Listeners[t_handle].get();

                    // iterate over listeners of the current handle
                    for (size_t j = 0; j < listeners->size(); j++) {
                        auto listener = listeners->at(j);

                        // check whether this is the listener we have created
                        if (listener->id == current_listener.id) {
                            // if yes, then remove it from the list of the
                            listeners->erase(listeners->begin() + j);
                            break;
                        }
                    }

                    // remove the vector from the map if it's empty
                    if (listeners->empty()) {
                        Process::Tcb->Wait_For_Listeners.erase(t_handle);
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
    auto process = (*Proc::Pcb)[handle];

    if (process != nullptr && process->status == Process_Status::Zombie) {
        exit_code = process->exit_code;

        std::string statuses[] = {
                "Ready", "Running", "Zombie"
        };

        // TODO remove for release, move to procfs
        printf("|%.*s|\n", 60, "========================================================================================================================");
        printf("|%-7s |%-10s |%-9s |%-7s |%-7s |%-10s|\n", "HANDLE", "PROGRAM", "STATUS", "STD_IN", "STD_OUT", "EXIT_CODE");
        printf("|%.*s|\n", 60, "========================================================================================================================");
        for (Process *p : (*Proc::Pcb).Get_Processes()) {
            printf("|%-7d |%-10s |%-9s |%-7d |%-7d |%-10hu|\n", p->handle, p->program_name, statuses[(int) p->status].c_str(), p->std_in, p->std_out, p->exit_code);
        }
        printf("|%.*s|\n", 60, "========================================================================================================================");

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
    if (process->signal_handlers[signal_id] != nullptr) {
        kiv_hal::TRegisters regs;
        regs.rcx.l = static_cast<decltype(regs.rcx.l)>(signal_id);
        // execute the handler of the given signal for the given process
        process->signal_handlers[signal_id](regs);
    }
}
