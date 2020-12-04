//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"
#include <stack>
#include <string>

extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    kiv_os::THandle handle;

    // directory file contains information about the directory itself, read the whole file
    auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));
    char* dir_to_be_read = (char*) regs.rdi.r;

    std::stack<std::string> path_stack;

    bool recursive = false;

    // load text to be echoed from rdi register
    char* args = (char*) regs.rdi.r;
//    if (strcmp(args, "/S") == 0) {
        recursive = true;
 //   }

    path_stack.push(std::string(dir_to_be_read));

    while (!path_stack.empty()) {
        auto path_to_open = path_stack.top();
        auto path_to_open_char_ptr = path_to_open.c_str();
        path_stack.pop();
        if (kiv_os_rtl::Open_File(path_to_open_char_ptr, 0, attributes, handle)) {
            // get the size of the TDir_Entry structure
            const auto dir_entry_size = sizeof(kiv_os::TDir_Entry);
            size_t read;

            // prepare in which we will read
            char read_buffer[dir_entry_size];

            // file_name size + 1 whitespace + 6 values in file_attributes + 1 \n + 1 NULL
            const auto out_buffer_size = sizeof(kiv_os::TDir_Entry::file_name) + 1 + 6 + 1 + 1;

            // prepare an out buffer
            char out_buffer[out_buffer_size];

            size_t written;

            // read directory items till EOF
            while (kiv_os_rtl::Read_File(handle, read_buffer, dir_entry_size, read)) {
                if (read == dir_entry_size) {
                    auto *entry = reinterpret_cast<kiv_os::TDir_Entry *>(read_buffer);
                    auto file_attributes = entry->file_attributes;

                    if (recursive &&
                        (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0) {
                        auto path = std::string(path_to_open_char_ptr) + "/" + std::string (entry->file_name);
                        printf("added %s to stack\n", path.c_str());
                        path_stack.push(path);
                    }

                    sprintf_s(out_buffer, out_buffer_size, "%-12s %d%d%d%d%d%d\n", entry->file_name,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive)) != 0
                    );

                    // out_buffer_size - 1 => sprintf_s concatenates with NULL but we don't want write NULL to std_out
                    kiv_os_rtl::Write_File(std_out, out_buffer, out_buffer_size - 1, written);
                }
            }

            kiv_os_rtl::Close_Handle(handle);
        } else {
            size_t written;
            const char *message = "Directory not found.\n";
            kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
        }
    }

    return 0;
}
