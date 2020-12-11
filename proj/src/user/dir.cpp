//
// Created by Stanislav Král on 31.10.2020.
//

#include "rtl.h"
#include <stack>
#include <string>
#include "error_handler.h"

extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs) {
    printf("CALLING DIR START\n\n\n\!!");

    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    kiv_os::THandle handle;

    // directory file contains information about the directory itself, read the whole file
    auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));

    // stack used to hold directory names to be processed
    std::stack<std::string> path_stack;

    // flag indicating whether printing of directories should be recursive (print all subdirectories)
    bool recursive = false;

    // maximum argument length
    const auto max_arg_length = 120;

    // variable used to store number of arguments provided
    uint8_t arg_c = 0;

    // array used to hold provided arguments
    char parsed_args[2][max_arg_length];

    // load text to be echoed from rdi register
    char* args = (char*) regs.rdi.r;

    // check whether arguments were provided
    if (regs.rdi.r != 0 && strlen(args) > 0) {
        // use whitespace as a separator
        const char separator[] = " ";

        // to be used in strtok_s
        char *token;
        char *next_token1 = NULL;

        // try to find the next token (argument)
        token = strtok_s(args, separator, &next_token1);

        if( token != NULL) {
            // an argument found, copy it to the args array
            strcpy_s(parsed_args[0], max_arg_length, token);
            arg_c++;

            // try to find another argument
            token = strtok_s(NULL, separator, &next_token1);
            if (token != nullptr) {
                // second argument found, copy it to the args array
                strcpy_s(parsed_args[1], max_arg_length, token);
                arg_c++;
            }
        }
    }

    if (arg_c > 0) {
        if (strcmp(parsed_args[0], "/S") == 0) {
            // S argument provided, print all subdirectories
            recursive = true;

            if (arg_c == 1) {
                // no path specified, print the current folder
                path_stack.push(".");
            } else {
                // both argument S and path provided
                path_stack.push(parsed_args[1]);
            }
        } else {
            // path provided
            path_stack.push(parsed_args[0]);
        }
    } else {
        // no path provided, print the current folder
        path_stack.push(".");
    }

    while (!path_stack.empty()) {
        auto path_to_open = path_stack.top();
        auto path_to_open_char_ptr = path_to_open.c_str();
        path_stack.pop();

        printf("BEFORE OPEN - IN DIR\n\n\n\!!");

        kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
        if ( kiv_os_rtl::Open_File(path_to_open_char_ptr, kiv_os::NOpen_File::fmOpen_Always, attributes, handle, error)) {

            printf("OPEN PASS\n\n\n\!!");

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

                printf("Handle is %d\n\n\n\!!");
                printf("Read buf cont - start\n");
                for (int i = 0; i < dir_entry_size; i++) {
                    printf("%c", read_buffer[i]);
                }
                printf("Read buf cont - end\n");


                if (read == dir_entry_size) {
                    auto *entry = reinterpret_cast<kiv_os::TDir_Entry *>(read_buffer);
                    auto file_attributes = entry->file_attributes;

                    if (recursive &&
                        (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0) {
                        auto path = std::string(path_to_open_char_ptr) + "/" + std::string (entry->file_name);
                        path_stack.push(path);
                    }

                    sprintf_s(out_buffer, out_buffer_size, "%.12s %u%u%u%u%u%u\n", entry->file_name,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0,
                              (file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive)) != 0
                    );

                    printf("Wanted to print: %.12s %u%u%u%u%u%u\n", entry->file_name,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only)) != 0,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden)) != 0,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File)) != 0,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) != 0,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0,
                        (file_attributes& static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive)) != 0
                    );

                    // out_buffer_size - 1 => sprintf_s concatenates with NULL but we don't want write NULL to std_out
                    kiv_os_rtl::Write_File(std_out, out_buffer, out_buffer_size - 1, written);
                }
            }

            kiv_os_rtl::Close_Handle(handle);
        } else {
            if (error == kiv_os::NOS_Error::File_Not_Found) {
                const auto mess_buff_size = 350;
                char mess_buff[mess_buff_size];
                sprintf_s(mess_buff, mess_buff_size, "Directory \"%s\" not found\n", path_to_open_char_ptr);

                size_t written;
                kiv_os_rtl::Write_File(std_out, mess_buff, strlen(mess_buff), written);
            } else {
                handle_error_message(error, std_out);
            }
        }
    }

    return 0;
}
