#pragma once

#include "..\api\api.h"
#include "rtl.h"
#include <set>
#include <string>

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters &regs);


//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!
extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs) { return 0; }
extern "C" size_t __stdcall rd(const kiv_hal::TRegisters &regs) { return 0; }
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall find(const kiv_hal::TRegisters &regs) { return 0; }
extern "C" size_t __stdcall sort(const kiv_hal::TRegisters & regs);
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters &regs) { return 0; }
void fill_supported_commands_set(std::set<std::string>&);
extern "C" size_t __stdcall charcnt(const kiv_hal::TRegisters & regs) { // todo remove this whole thing + name from user.def
    size_t counter;
    const size_t buffer_size = 256;
    char buffer[buffer_size];
    const char* new_line = "\n";

    // memset the buffer to empty, just to be safe:
    memset(buffer, 0, buffer_size);

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    int char_count = 0;

    bool doContinue = true; // flag to tell if we should break out of the reading cycle
    do {
        if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter)) {

            for (int i = 0; i < counter; i++) {

                // check for EOT:
                if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                    doContinue = false;
                    break; // EOT
                }

                char_count++;
            }

        }
        else {
            break; // EOF
        }

    } while (doContinue);


    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);

    size_t n = 0;
    memset(buffer, 0, buffer_size);

    // format it in the way that was requested:
    n = sprintf_s(buffer, "Number of characters is : %d", char_count);

    kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), n);
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);

    return 0;
}
