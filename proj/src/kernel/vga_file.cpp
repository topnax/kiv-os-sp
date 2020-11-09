//
// Created by Stanislav Král on 06.11.2020.
//

#include <cstdio>
#include "vga_file.h"
#include "../api/hal.h"

size_t Vga_File::write(char *buffer, size_t size) {
    kiv_hal::TRegisters registers;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);

    registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(buffer);
    registers.rcx.r = size;

    //preklad parametru dokoncen, zavolame sluzbu
    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);

    auto error = registers.rax.r == 1;

    // TODO error?

    return size; //VGA BIOS nevraci pocet zapsanych znaku, tak predpokladame, ze zapsal vsechny
}

size_t Vga_File::read(size_t size, char *out_buffer) {
    // cannot read from VGA
    return -1;
}
