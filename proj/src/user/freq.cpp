//
// Created by Eliška Mourycová on 16.10.2020.
//

#include <thread>
#include "../api/hal.h"
#include "rtl.h"
#include <vector>

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters & regs) {
    size_t counter;
    const size_t buffer_size = 256;
    char buffer[buffer_size];
    const char* new_line = "\n";

    // two arrays for remembering the frequencies of chars: 
    std::vector<unsigned char> chars;
    std::vector<unsigned int> frequencies;

    // memset the buffer to empty, just to be safe:
    memset(buffer, 0, buffer_size);

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);



    bool found = false;     // flag to keep track of if a char has been encountered
    bool doContinue = true; // flag to tell if we should break out of the reading cycle
    do {
        if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter)) {

            //if (counter < buffer_size) {
            //    doContinue = false; // this is here so that we quit when there is nothing more to read - ok?
            //}

            // build the frequency table:
            for (int i = 0; i < counter; i++) {
                found = false; // reset the flag

                // check for EOT:
                if (static_cast<kiv_hal::NControl_Codes>(buffer[i]) == kiv_hal::NControl_Codes::EOT) {
                    doContinue = false;
                    break; // EOT
                }

                // go through the chars to see if we've got the current one stored already:
                for (int j = 0; j < chars.size(); j++) {

                    if (buffer[i] == chars[j]) {
                        // if we know this char, only increase the frequency and set found flag to true
                        frequencies[j]++;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // if we haven't found anything, add new record with a frequency set to 1
                    frequencies.push_back(1);
                    chars.push_back(buffer[i]);
                }
                
            }
        }
        else {
            break; // EOF
        }
            
    } while (doContinue);

    
    kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);


    
    // printing the gathered frequencies:
    size_t n = 0;

    for (int i = 0; i < chars.size(); i++) {
        
        memset(buffer, 0, buffer_size);
        // format it in the way that was assigned:
        n = sprintf_s(buffer, "0x%hhx : %d", chars[i], frequencies[i]); 


        kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), n);
        kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
    }

    return 0;
}
