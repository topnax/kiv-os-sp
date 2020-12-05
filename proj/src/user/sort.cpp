//
// Created by Eliska Mourycova on 28.11.2020
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include "argparser.h"
#include <vector>
#include <string>
#include <algorithm>
#include <locale>
#include "error_handler.h"

extern "C" size_t __stdcall sort(const kiv_hal::TRegisters & regs) {
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
    bool closeFileHandle = true;

    if (data && strlen(data) > 0) {
        // we will be reading from a file
        strtrim(data);
        //auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only));
        kiv_os::NOS_Error error;
        if (kiv_os_rtl::Open_File(data, kiv_os::NOpen_File::fmOpen_Always, 0, file_handle, error)) {
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
                    break; // EOT
                }

                auto c = buffer[i];
                //input_vec.push_back(c);

                if (c == '\n') {
                    lines.push_back(curr_string);
                    curr_string = "";
                    continue;
                }
                curr_string += c;
            }
            // kiv_os_rtl::Close_Handle(std_in);
        }
        else {
            doContinue = false;
            break; // EOF
        }

    } while (doContinue);
    if(curr_string.size() > 0)
        lines.push_back(curr_string); // the last line does not have to end with \n

    // sorting the lines - using locale argument so that uppercase appears after lowercase
    std::sort(lines.begin(), lines.end(), std::locale("en_US.UTF-8"));

    // output the text
    for (int i = 0; i < lines.size(); i++) {
        //kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
        kiv_os_rtl::Write_File(std_out, lines[i].c_str(), lines[i].length(), counter);
        kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
    }

    // append with new line
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
    
    if(closeFileHandle)
        kiv_os_rtl::Close_Handle(file_handle);

    return 0;
}
