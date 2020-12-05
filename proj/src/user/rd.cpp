//
// Created by Stanislav Král on 05.12.2020.
//

#include "../api/hal.h"
#include "../api/api.h"
#include "rtl.h"
#include "error_handler.h"

extern "C" size_t __stdcall rd(const kiv_hal::TRegisters &regs) {
    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // load the directory name from rdi register
    char *directory_name = reinterpret_cast<char *>(regs.rdi.r);

    kiv_os::NOS_Error error;

    if (!kiv_os_rtl::Delete_File(directory_name, error)) {
        // syscall failed - directory not deleted
        handle_error_message(error, std_out);
    }

    return 0;
}
