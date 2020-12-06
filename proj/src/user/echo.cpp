//
// Created by Stanislav Král on 11.10.2020.
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) {
    size_t counter;

    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // load text to be echoed from rdi register
    char* text = (char*) regs.rdi.r;

    // new line
    const char *new_line = "\n";

    // windows cmd doesnt behave like this
    if (text[0] == '\"' && text[strlen(text) - 1] == '\"') {
        // if the text starts and ends with double quotes - remove them
        text++;
        text[strlen(text) - 1] = '\0';
    }

    // output the text
    kiv_os_rtl::Write_File(std_out, text, strlen(text), counter);

    // append with new line
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);

    return 0;
}
