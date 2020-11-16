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

    // TODO remove this in release
    if (strcmp(text, "delayed_echo") == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    // TODO remove for release
    if (strcmp(text, "test_exit") == 0) {
        kiv_os_rtl::Exit(kiv_os::NOS_Error::IO_Error);
    } else {
        // output the text
        kiv_os_rtl::Write_File(std_out, text, strlen(text), counter);

        // append with new line
        //kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
    }

    return 0;
}
