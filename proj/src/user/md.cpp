//
// Created by Stanislav Král on 05.12.2020.
//

#include "../api/hal.h"
#include "../api/api.h"
#include "rtl.h"
#include "error_handler.h"

extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs) {

    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // load the directory name from rdi register
    char *directory_name = reinterpret_cast<char *>(regs.rdi.r);

    if (directory_name == nullptr || strlen(directory_name) == 0) {
        const char *error_message = "Name not specified.\n";
        size_t written;
        kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
    } else {
        // do not set NOpen_File flag - the file has to be created
        auto flags = static_cast<kiv_os::NOpen_File>(0);

        // set the file type to directory
        auto attributes = static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory);

        kiv_os::THandle handle;
        kiv_os::NOS_Error error;
        if (kiv_os_rtl::Open_File(directory_name, flags, attributes, handle, error)) {
            // directory created, close the handle
            kiv_os_rtl::Close_Handle(handle);
        } else {
            if (error == kiv_os::NOS_Error::Invalid_Argument) {
                const char *error_message = "File already exists or invalid name specified.\n";
                size_t written;
                kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            } else {
                handle_error_message(error, std_out);
            }
        }
    }

    return 0;
}
