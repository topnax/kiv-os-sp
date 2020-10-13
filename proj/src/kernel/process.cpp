//
// Created by Stanislav Král on 11.10.2020.
//
#include <iostream>
#include <thread>
#include "../api/hal.h"
#include "../api/api.h"

void clone(kiv_hal::TRegisters &registers);

void Handle_Process(kiv_hal::TRegisters &regs) {
    switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

        case kiv_os::NOS_Process::Clone: {
            clone(regs);
        }
            break;
    }
}

void program_entrypoint(kiv_hal::TRegisters &registers) {
    // call TThread_Proc ()
    ((kiv_os::TThread_Proc) registers.rdx.r)(registers);
}

void clone(kiv_hal::TRegisters &registers) {
    std::thread t1(program_entrypoint, registers);
    // TODO do not join here, that should be done using WaitFor syscall
    t1.join();
}
