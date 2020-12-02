//
// Created by Stanislav Král on 02.12.2020.
//
#include <filesystem>
#include "../api/api.h"

bool Set_Working_Dir(kiv_hal::TRegisters &regs);

/**
 * Stores the working dir of the current process into the provided registers
 */
bool Get_Working_Dir(kiv_hal::TRegisters &regs);

/**
 * Stores the working dir of the current process into the given `out_buffer` and sets the `written` variable to the amount of bytes written to the buffer
 */
bool Get_Working_Dir(char *out_buffer, size_t buffer_size, size_t &written);

/**
 * Stores the working dir of the current process into the given path
 */
bool Get_Working_Dir(std::filesystem::path &out);
