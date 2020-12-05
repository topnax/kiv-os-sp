//
// Created by Stanislav Král on 05.12.2020.
//

#include "../api/hal.h"
#include "../api/api.h"
#include "rtl.h"

extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs) {

    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // load the directory name from rdi register
    char *directory_name = reinterpret_cast<char *>(regs.rdi.r);

    // do not set NOpen_File flag - the file has to be created
    auto flags = static_cast<kiv_os::NOpen_File>(0);

    // set the file type to directory
    auto attributes = static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory);

    kiv_os::THandle handle;
    if (kiv_os_rtl::Open_File(directory_name, flags, attributes, handle) && handle != kiv_os::Invalid_Handle) {
        // directory created, close the handle
        kiv_os_rtl::Close_Handle(handle);
    } else {
        const char *error = "File already exists, directory not created.\n";
        size_t written;
        kiv_os_rtl::Write_File(std_out, error, strlen(error), written);
    }

    return 0;
}
