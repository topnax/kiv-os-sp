//
// Created by Eliska Mourycova on 30.11.2020
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include <vector>
#include <string>
//#include <iostream>
#include <algorithm>
#include <locale>

extern "C" size_t __stdcall find(const kiv_hal::TRegisters & regs) {
    size_t counter;

    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    const size_t buffer_size = 256;
    char buffer[buffer_size];

    // load the arg from rdi register
    char* data = (char*)regs.rdi.r;

    // new line
    const char* new_line = "\n";

    //std::vector<char> input_vec;
    kiv_os::THandle file_handle;
    std::vector<std::string> lines;
    std::string curr_string = "";

    printf("find start \n");
    int lines_count = 0;
    char last_char = '\0';

    if (kiv_os_rtl::Open_File(data, 0, 0, file_handle)) {
        size_t read;
        char buff[buffer_size];
        size_t written;

        // read till EOF
        while (kiv_os_rtl::Read_File(file_handle, buff, buffer_size, read)) {
            //kiv_os_rtl::Write_File(std_out, buff, read, written);
            for (int i = 0; i < read; i++) {
                if (buff[i] == '\n') {
                    lines_count++;
                }
            }
            if(read > 0)
                last_char = buff[read - 1];
        }
        // eof happened
        if (last_char != '\n') {
            lines_count++; // todo this might be wrong, e.g for completely empty file we get 1 line
        }
        memset(buff, 0, buffer_size);
        size_t n = sprintf_s(buff, "Number of lines is : %d", lines_count);
        kiv_os_rtl::Write_File(std_out, buff, n, written);
        kiv_os_rtl::Write_File(std_out, "\n", 1, written);
        kiv_os_rtl::Close_Handle(file_handle);
    }
    else {
        size_t written;
        char* message = "File not found\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
    }
    return 0;
}
