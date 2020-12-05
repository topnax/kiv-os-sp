//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"

extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) {
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // flag indicating whether the file to be read has not been found
    bool handle_found = false;

    // flag indicating whether a file has been opened
    bool file_opened = false;

    // the handle to be used to read from
    kiv_os::THandle handle_to_read_from;

    char *file_path = (char *) regs.rdi.r;
    // check whether user has specified a file to read from
    if (regs.rdi.r != 0 && strlen(file_path) > 0) {
        // get path of the file to be read

        // try to open a file at the specified path
        if (kiv_os_rtl::Open_File(file_path, kiv_os::NOpen_File::fmOpen_Always, 0, handle_to_read_from)) {
            file_opened = true;
            handle_found = true;
        } else {
            size_t written;
            const char *message = "File not found\n";
            kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
        }
    } else {
        // a file to be read from not specified, read from std_in
        handle_found = true;
        handle_to_read_from = std_in;
    }

    // any file has been found
    if (handle_found) {
        // define the size of the read buffer
        const size_t buffer_size = 100;
        char buff[buffer_size];

        size_t read;
        size_t written;

        // TODO eof checking might be in kernel?
        // read till EOF
        bool eot_found = false;
        while (!eot_found && kiv_os_rtl::Read_File(handle_to_read_from, buff, buffer_size, read)) {
            for (int i = 0; i < read; i++) {
                // check whether EOT has been read, if so, then abort reading from the handle and do not print EOT char
                if (static_cast<kiv_hal::NControl_Codes>(buff[i]) == kiv_hal::NControl_Codes::EOT) {
                    eot_found = true;
                    read = i;
                    break;
                }
            }
            kiv_os_rtl::Write_File(std_out, buff, read, written);
        }

        // TODO should we write \n?
        kiv_os_rtl::Write_File(std_out, "\n", 1, written);
        // close only if opening a file
        if (file_opened) {
            kiv_os_rtl::Close_Handle(handle_to_read_from);
        }
    }

    return 0;
}
