//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters &regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    kiv_os::THandle handle;

    // /procfs/tasklist provides information about running processes, read the whole file

    auto attributes =
            static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File);

    // TODO use change file attributes?
    if (kiv_os_rtl::Open_File("/procfs/tasklist", 0, attributes, handle)) {
        const auto buff_size = 100;
        size_t read;
        char buff[buff_size];
        size_t written;

        // read till EOF
        while (kiv_os_rtl::Read_File(handle, buff, buff_size, read)) {
            kiv_os_rtl::Write_File(std_out, buff, read, written);
        }

        kiv_os_rtl::Write_File(std_out, "\n", 1, written);
        kiv_os_rtl::Close_Handle(handle);
    } else {
        size_t written;
        char *message = "Could not open PCB table\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
    }

    return 0;
}
