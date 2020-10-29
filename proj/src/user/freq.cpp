//
// Created by Eliška Mourycová on 16.10.2020.
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include <vector>

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters & regs) {
    size_t counter;
    const size_t buffer_size = 256; // TODO, is this enough? Can we somehow read chars as they come from the user?
    char buffer[buffer_size];
    const char* new_line = "\n";
    size_t one = 1;

    //std::map<unsigned char, int> freq_table;
    std::vector<unsigned char> chars;
    std::vector<unsigned int> frequencies;

    memset(buffer, 0, buffer_size);

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    // read from std_in:
    kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter);

    //kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);
    //kiv_os_rtl::Write_File(std_out, buffer, counter, counter);
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);


    // build the frequency table:
    bool found = false;
    for (int i = 0; i < counter; i++) {
        found = false;

        for (int j = 0; j < chars.size(); j++) {

            if (buffer[i] == chars[j]) {
                frequencies[j]++;
                found = true;
                break;
            }
        }
        if (!found) {
            frequencies.push_back(1);
            chars.push_back(buffer[i]);
        }
    }
    

    // printing the gathered frequencies
    size_t n = 0;

    for (int i = 0; i < chars.size(); i++) {
        
        memset(buffer, 0, 10);
        n = sprintf_s(buffer, "0x%hhx : %d", chars[i], frequencies[i]);


        kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), n);
        kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);
    }

    return 0;
}
