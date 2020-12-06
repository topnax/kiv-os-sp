//
// Created by Eliska Mourycova on 30.11.2020
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include "argparser.h"
#include <vector>
#include <string>
//#include <iostream>
#include <algorithm>
#include <locale>
#include "error_handler.h"

extern "C" size_t __stdcall find(const kiv_hal::TRegisters & regs) {
    size_t counter;

    // get a reference to std_out
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

    const size_t buffer_size = 256;
    char buffer[buffer_size];

    memset(buffer, 0, buffer_size);

    // load the arg from rdi register
    char* data = (char*)regs.rdi.r;

    // new line
    const char* new_line = "\n";

    //std::vector<char> input_vec;
    kiv_os::THandle file_handle;
    std::vector<std::string> lines;
    std::string curr_string = "";

    int lines_count = 0;
    char last_char = '\0';
    bool closeFileHandle = true;


    // parse the data here:
    // the format is find /v /c "" file.txt
    std::string form = "/v /c \"\"";
    int pos = 0;
    std::string args = data;
    int index = static_cast<int>(args.find(form, pos));
    if (index == 0) {
        // the format is ok, now separate the file path:
        args = args.substr(form.size(), args.size() - 1);
        args = trim(args, " ");
    }
    else {
        size_t written;
        char* message = "Arguments expected: find /v /c \"\" {optional/path/to/file}\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
        return 0;
    }


    if (args.c_str() && strlen(args.c_str()) > 0) {
        // we will be reading from a file

        kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
        //auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only));
        if (kiv_os_rtl::Open_File(args.c_str(), kiv_os::NOpen_File::fmOpen_Always, 0, file_handle, error)) {
            ;;
        }
        else {
            handle_error_message(error, std_out);
            return 0; // 0?
        }
    }
    else {
        // we will be reading from std input
        file_handle = std_in;
        closeFileHandle = false;
    }

    bool doContinue = true; // flag to tell if we should break out of the reading cycle
    do {
        if (kiv_os_rtl::Read_File(file_handle, buffer, buffer_size, counter)) {
            // TODO uncomment this for newline addition in user's input of text:
            //if (counter < buffer_size) {
            //    // this happens on Enter?
            //    size_t written = 0;
            //    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), written); // write the new line
            //    buffer[counter] = '\n'; // add the char to the buffer
            //    counter++; // increase the counter, bc we added one char (\n)
            //}


            for (int i = 0; i < counter; i++) {

                // check for EOT:
                if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                    doContinue = false;
                    //if (last_char != '\n') {
                    //    lines_count++; // todo this might be wrong, e.g for completely empty file we get 1 line
                    //}
                    break; // EOT
                }

                if (buffer[i] == '\n') {
                    lines_count++;
                }
            }
            if (counter > 0)
                last_char = buffer[counter - 1];
        }
        else {
            doContinue = false;
            //if (last_char != '\n') {
            //    lines_count++; // todo this might be wrong, e.g for completely empty file we get 1 line
            //}
            break; // EOF
        }

    } while (doContinue);


    memset(buffer, 0, buffer_size);
    //size_t n = sprintf_s(buff, "Number of lines is : %d", lines_count);
    size_t n = sprintf_s(buffer, "%d", lines_count);
    size_t written = 0;
    kiv_os_rtl::Write_File(std_out, buffer, n, written);
    kiv_os_rtl::Write_File(std_out, "\n", 1, written);

    if(closeFileHandle)
        kiv_os_rtl::Close_Handle(file_handle);


    return 0;
}
