//
// Created by Stanislav Král on 06.11.2020.
//

#include <cstdio>
#include "Keyboard_File.h"
#include "../api/hal.h"

size_t Keyboard_File::write(char *buffer, size_t size) {
   // kiv_hal::TRegisters registers;

   // size_t pos = 0;
   // while (pos < size) {
   //     char to_write = buffer[pos];
   //     registers.rax.x = to_write;

   //     registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Write_Char);
   //     kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);
   //     if (registers.flags.non_zero) {
   //         pos++;
   //     } else {
   //         break;
   //     }
   // }
    // return pos;

    return -1; // cannot write to keyboard
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
                registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
                registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
                registers.rcx.r = 1;
                kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
                break;
        }
    }

    return pos;

}

size_t Keyboard_File::read(size_t size, char *out_buffer) {
    return Read_Line_From_Console(reinterpret_cast<char*>(out_buffer), size);
}