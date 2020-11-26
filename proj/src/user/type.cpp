//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"

extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) {
    const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    kiv_os::THandle handle;

    // get path of the file to be read
    char *file_path = (char *) regs.rdi.r;

    const size_t buffer_size = 100;
    // TODO use file attributes?
    if (kiv_os_rtl::Open_File(file_path, 0, 0, handle)) {
        size_t read;
        char buff[buffer_size];
        size_t written;

        // read till EOF
        while (kiv_os_rtl::Read_File(handle, buff, buffer_size, read)) {
            kiv_os_rtl::Write_File(std_out, buff, read, written);
        }

        kiv_os_rtl::Write_File(std_out, "\n", 1, written);
        kiv_os_rtl::Close_Handle(handle);
    } else {
        size_t written;
        char *message = "File not found\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
    }
    return 0;
}
