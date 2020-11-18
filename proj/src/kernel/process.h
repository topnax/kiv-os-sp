#pragma once

#include "..\api\api.h"

#include "Windows.h"
#include "map"
#include "memory"
#include "vector"
#include "semaphore.h"

/**
 *  A structure representing a Wait_For listener waiting for a thread to notify it
 */
struct Wait_For_Listener {
    /**
     * Thread to be notified once any specified thread finishes
     */
    Semaphore *semaphore;

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
};

void thread_post_execute(bool is_process);

void Handle_Process(kiv_hal::TRegisters &regs, HMODULE user_programs);

void clone(kiv_hal::TRegisters &registers, HMODULE user_programs);

void wait_for(kiv_hal::TRegisters &registers);

void exit(kiv_hal::TRegisters &registers);

void exit(kiv_os::THandle handle, kiv_os::NOS_Error exit_code);

void read_exit_code(kiv_hal::TRegisters &registers);
