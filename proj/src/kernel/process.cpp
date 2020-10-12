//
// Created by Stanislav Král on 11.10.2020.
//
#include <iostream>
#include "../api/hal.h"
#include "../api/api.h"

void Handle_Process(kiv_hal::TRegisters &regs) {
    std::cout << "handling a process syscall" << std::endl;

    switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

        case kiv_os::NOS_Process::Clone: {
            // TODO clone
            std::cout << "handling a clone syscall" << std::endl;
        }
            break;
    }
}
