//
// Created by Stanislav Král on 31.10.2020.
//

#include <thread>
#include <random>
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
            for (int i = 0; i < counter; i++) {
                switch (uint8_t(buffer[i])) {
                    // TODO remove for release - CLion terminal is quirky and sending EOT does not work correctly there
                    case uint8_t('E'):
                    case uint8_t(kiv_hal::NControl_Codes::EOT):
                    case uint8_t(kiv_hal::NControl_Codes::ETX):
                        terminate = true;
                }
            }
        } else {
            terminate = true;
        }

    } while (!terminate);

    // check whether rgen is still generating
    // rgen might have ended (failed to write to std_out) and the pointer to run could be invalid
    if (*parameters->is_generating) {
        // rgen should stop generating
        *generate = false;
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
    kiv_os::THandle handle;
    kiv_os_rtl::Clone_Thread(stdin_guard, reinterpret_cast<char *>(&parameters), handle);

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(-128, 127); // define the range

    size_t counter;
    while (*generate) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
        int random_number = distr(gen); // generate a random number
        char random_char = (char) random_number; // convert it to a char

        if (static_cast<kiv_hal::NControl_Codes>(random_char) == kiv_hal::NControl_Codes::EOT) {
            continue;
        }

        // write to std_out (1 char)
        if (!kiv_os_rtl::Write_File(std_out, &random_char, 1, counter)) {
            // could not write to file, abort
            is_generating = false;
            break;
        }
    }

    if (!is_generating && *generate) {
        // TODO out handle might have been closed, notify the guard thread

        // this should shutdown the guard thread, but keyboard does not support writing characters to it
        // there is no way of stopping the guard thread at this moment

        // char eot = static_cast<char>(kiv_hal::NControl_Codes::EOT);
        //kiv_os_rtl::Write_File(std_in, &eot, 1, counter);
    }

    return 0;
}
