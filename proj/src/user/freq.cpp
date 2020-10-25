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
    char buffer2[buffer_size];
    char small_buff[10];
    const char* new_line = "\n";
    size_t one = 1;
    //std::map<unsigned char, int> freq_table;
    std::vector<unsigned char> chars;
    std::vector<int> frequencies;

    //// get a reference to std_out
    //const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    //// load text to be echoed from rdi register
    //char* text = (char*)regs.rdi.r;

    //// new line
    //const char* new_line = "\n";

    //// TODO remove this in release
    //if (strcmp(text, "delayed_echo") == 0) {
    //    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //}
    //// output the text
    //kiv_os_rtl::Write_File(std_out, text, strlen(text), counter);

    //// append with new line
    //kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    // read from std_in:
    kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter);

    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);
    kiv_os_rtl::Write_File(std_out, buffer, counter, counter);
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);


    memcpy(buffer2, buffer, strlen(buffer));

    // build the frequency table:
    for (int i = 0; i < counter; i++) {
        //chars.push_back(buffer[i]);
        frequencies.push_back(1);

        for (int j = i + 1; j < counter; j++) {
            if (buffer[i] == buffer[j]) {
                frequencies[i]++;
                
            }
        }
    }

    // printing the gathered frequencies
    for (int i = 0; i < chars.size(); i++) {
        small_buff[0] = chars[i];
        kiv_os_rtl::Write_File(std_out, small_buff, strlen(small_buff), one);
        kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), one);
    }

    return 0;
}
