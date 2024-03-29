#pragma once

#include "..\api\api.h"

#include "Windows.h"
#include "map"
#include "memory"
#include "vector"
#include "semaphore.h"
#include "process/process_entry.h"
#include "process/pcb.h"

/**
 *  A structure representing a Wait_For listener waiting for a thread to notify it
 */
struct Wait_For_Listener {
    /**
     * ID of this listener
     */
     kiv_os::THandle id;

    /**
     * Semaphore to be notified once any specified thread finishes
     */
    std::unique_ptr<Semaphore> semaphore;

    /**
     * An attribute containing the handle of the thread that has notified the semaphore
     */
    kiv_os::THandle handle_that_notified;

    /**
     * A flag indicating whether this listener has been notified
     */
    bool notified;
};

class Thread_Control_Block {
public:
    std::map<kiv_os::THandle, std::unique_ptr<std::vector<Wait_For_Listener *>>> Wait_For_Listeners;
    std::map<kiv_os::THandle, kiv_os::THandle> Parent_Processes;
};

bool resolve_native_thread_id_to_thandle(std::thread::id native_thread_id, kiv_os::THandle &out_handle);

void thread_post_execute(bool is_process);

void Handle_Process(kiv_hal::TRegisters &regs, HMODULE user_programs);

void clone(kiv_hal::TRegisters &registers, HMODULE user_programs);

void wait_for(kiv_hal::TRegisters &registers);

void exit(kiv_hal::TRegisters &registers);

void exit(kiv_os::THandle handle, kiv_os::NOS_Error exit_code);

void read_exit_code(kiv_hal::TRegisters &registers);

void register_signal_handler(kiv_hal::TRegisters &registers);

void signal_all_processes(kiv_os::NSignal_Id signal);

void signal(kiv_os::NSignal_Id signal_id, Process *process);

void shutdown();

Process_Control_Block* Get_Pcb();

Thread_Control_Block* Get_Tcb();

Process* resolve_current_thread_handle_to_process();
