//
// Created by Eliška Mourycová on 16.10.2020.
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include <vector>

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs) {
    size_t counter;
    const size_t buffer_size = 256;
    char buffer[buffer_size];
    const char *new_line = "\n";

    // an array for remembering the frequencies of chars - the index coresponds to the char: 
    std::vector<unsigned int> frequencies(256);

    // memset the buffer to empty, just to be safe:
    memset(buffer, 0, buffer_size);

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    bool doContinue = true; // flag to tell if we should break out of the reading cycle
    do {
        if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter)) {
            // build the frequency table:
            for (int i = 0; i < counter; i++) {

                // check for EOT:
                if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                    doContinue = false;
                    break; // EOT
                }

                // increase the frequency of current char
                auto c = buffer[i];
                frequencies[static_cast<size_t>(c) + 128]++;
            }
            // kiv_os_rtl::Close_Handle(std_in);
        } else {
            break; // EOF
        }

    } while (doContinue);


    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);


    // printing the gathered frequencies from 0x00 to 0xff:
    size_t n = 0;
    //int c = -1;
    for (int i = 128; i < frequencies.size(); i++) {
        if (frequencies[i] > 0) {
            memset(buffer, 0, buffer_size);

            // format it in the way that was requested:
            n = sprintf_s(buffer, "0x%hhx : %u", i - 128, frequencies[i]);

            kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), n);
            // stop the loop if we failed to write
            if (n == 0) break;
            kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
            if (counter == 0) break;
        }
    }

    for (int i = 0; i <= 127; i++) {
        if (frequencies[i] > 0) {
            memset(buffer, 0, buffer_size);

             // format it in the way that was requested:
            n = sprintf_s(buffer, "0x%hhx : %u", i + 128, frequencies[i]);

            kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), n);
            // stop the loop if we failed to write
            if (n == 0) break;
            kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
            if (counter == 0) break;
        }
    }

    return 0;
}
