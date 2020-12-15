//
// Created by Stanislav Král on 31.10.2020.
//

#include <thread>
#include <random>
#include <limits>
#include "rtl.h"

struct Rgen_Stdin_Guard_Parameters {
    bool *generate;
    kiv_os::THandle std_in;
    bool *is_generating;
};

extern "C" size_t __stdcall stdin_guard(const kiv_hal::TRegisters &regs) {
    // get the pointer to the flag to be set once EOT, ETX is read
    auto parameters = reinterpret_cast<Rgen_Stdin_Guard_Parameters *>(regs.rdi.r);

    // load the run flag pointer from parameters
    bool *generate = parameters->generate;

    // get the std_in handle
    const auto std_in = parameters->std_in;

    // create a read buffer
    const size_t buffer_size = 256;
    char buffer[buffer_size];
    size_t counter;

    bool terminate = false;
    do {
        bool res = kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter);
        if (res) {
            for (size_t i = 0; i < counter; i++) {
                switch (uint8_t(buffer[i])) {
                    case uint8_t(kiv_hal::NControl_Codes::EOT):
                    case uint8_t(kiv_hal::NControl_Codes::ETX):
                        terminate = true;
                }
            }
        } else {
            terminate = true; // EOF
        }

    } while (!terminate);

    // check whether rgen is still generating
    // rgen might have ended (failed to write to std_out) and the pointer to run could be invalid
    if (*parameters->is_generating) {
        // rgen should stop generating
        *generate = false; // comment out this if testing shutdown
    }
    return 0;
}

extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {

    // get a references to std_in and std_out
    const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // share a pointer to a boolean flag indicating whether rgen should still be running
    auto generate = std::make_unique<bool>(true);

    // share a pointer to a boolean flag indicating whether rgen is still running (might be aborted if std_out is closed)
    bool is_generating = true;

    // parameters for the guard thread
    auto parameters = Rgen_Stdin_Guard_Parameters{
            generate.get(),
            std_in,
            &is_generating
    };

    // spawn a thread that checks whether EOT has been written to stdin
    kiv_os::THandle guard_handle;
    if (!kiv_os_rtl::Clone_Thread(stdin_guard, reinterpret_cast<char *>(&parameters), guard_handle)) {
        kiv_os_rtl::Exit(kiv_os::NOS_Error::Out_Of_Memory);
        return 0;
    }

    // TODO random_device causes a memory leak, move it to a static namespace?
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    //std::uniform_int_distribution<> distr(-128.0, 127.0); // define the range
    //std::uniform_real_distribution<> distr(std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
    std::uniform_real_distribution<> distr(-100.0, 100.0);

    size_t counter;
    bool res;
    char string[99];
    while (*generate) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto random_number = static_cast<float>(distr(gen)); // generate a random number
        
        
        memset(string, 0, 99);
        sprintf_s(string, "%.25lf", random_number);

        res = kiv_os_rtl::Write_File(std_out, string, strlen(string), counter);
        if (!res) {
            // could not write to file, abort
            is_generating = false;
            break;
        }

        
    }

    //check whether std_in guard has not yet signalled us to stop generating
    //and check whether we have stopped generating due to write failure
    if (*generate && !is_generating) {
        // write EOT to the std_in of the guard thread, signalling it to end
        char eot = static_cast<char>(kiv_hal::NControl_Codes::EOT);
        kiv_os_rtl::Write_File(std_in, &eot, 1, counter);

        // wait for the guard to finish
        kiv_os::THandle handles[] = {guard_handle};
        uint8_t index = 0;
        kiv_os_rtl::Wait_For(handles, 1, index);
    }

    return 0;
}
