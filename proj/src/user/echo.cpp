//
// Created by Stanislav Král on 11.10.2020.
//

#include <iostream>
#include "../api/hal.h"

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) {
    std::wcout << "Hello From echo.cpp hi" << std::endl;
    return 0;
}
