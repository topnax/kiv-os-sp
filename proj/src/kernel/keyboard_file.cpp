//
// Created by Stanislav Král on 06.11.2020.
//

#include <cstdio>
#include "keyboard_file.h"
#include "../api/hal.h"

bool Keyboard_File::write(char *buffer, size_t size, size_t &written) {
    kiv_hal::TRegisters registers;

    size_t pos = 0;
    while (pos < size) {
        char to_write = buffer[pos];
        registers.rax.x = static_cast<decltype(registers.rax.l)>(to_write);
        registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Write_Char);
        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);
        if (registers.flags.non_zero) {
            pos++;
        } else {
            // an error has occurred
            return false;
        }
    }
   return true;
}

size_t Keyboard_File::Read_Line_From_Console(char *buffer, const size_t buffer_size) {
    kiv_hal::TRegisters registers;

    size_t pos = 0;
    while (pos < buffer_size) {
        //read char
        registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Read_Char);
        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);

        if (!registers.flags.non_zero) break;	//nic jsme neprecetli,
        //pokud je rax.l EOT, pak byl zrejme vstup korektne ukoncen
        //jinak zrejme doslo k chybe zarizeni

        char ch = registers.rax.l;

        //osetrime zname kody
        switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
            case kiv_hal::NControl_Codes::BS: {
                //mazeme znak z bufferu
                if (pos > 0) pos--;

                registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
                registers.rdx.l = ch;
                kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
            }
                break;

            case kiv_hal::NControl_Codes::LF:  break;	//jenom pohltime, ale necteme
            case kiv_hal::NControl_Codes::NUL:			//chyba cteni?
            case kiv_hal::NControl_Codes::CR:  return pos;	//docetli jsme az po Enter


            default: buffer[pos] = ch;
                pos++;
                if (static_cast<kiv_hal::NControl_Codes>(ch) != kiv_hal::NControl_Codes::EOT) {
                    registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
                    registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
                    registers.rcx.r = 1;
                    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
                } else {
                    // EOT has been read, stop waiting for remaining characters
                    eot_read = true;
                    return pos;
                }
                break;
        }
    }

    return pos;

}

bool Keyboard_File::read(size_t size, char *out_buffer, size_t &read) {
    if (eot_read) {
        return false;
    }
    read = Read_Line_From_Console(reinterpret_cast<char*>(out_buffer), size);
    // TODO should we always return true?
    return true;
}