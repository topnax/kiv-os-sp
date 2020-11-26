//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"

extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    kiv_os::THandle handle;

    // directory file contains information about the directory itself, read the whole file
    auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));
    char* dir_to_be_read = (char*) regs.rdi.r;
    // TODO change file attributes
    if (kiv_os_rtl::Open_File(dir_to_be_read, 0, attributes, handle)) {
        const auto dir_entry_size = sizeof(kiv_os::TDir_Entry);
        size_t read;
        char buff[dir_entry_size];
        size_t written;

        // TODO VFS->generate_dir_vector should generate raw TDir_Entry structures and DIR should parse it

        // read directory items till EOF
        while(kiv_os_rtl::Read_File(handle, buff, dir_entry_size, read)) {
            kiv_os_rtl::Write_File(std_out, buff, read, written);
        }

        kiv_os_rtl::Write_File(std_out, "\n", 1, written);
        kiv_os_rtl::Close_Handle(handle);
    } else {
        size_t written;
        char *message = "Directory not found.\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
    }

    return 0;
}
