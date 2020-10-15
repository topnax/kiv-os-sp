#include <iostream>
#include "shell.h"
#include "rtl.h"

void
call_program(size_t (__stdcall tthread_proc)(const kiv_hal::TRegisters &regs), const kiv_hal::TRegisters &pRegisters,
             char *data) {
    // call clone from RTL
    // RTL is used we do not have to set register values here

    // TODO remove such test in release
    if (strcmp(data, "test_handles") == 0) {
        kiv_os::THandle handle2;
        char* test = "delayed_echo";
        kiv_os_rtl::Clone(static_cast<kiv_os::TThread_Proc>(echo), test, handle2);

        // the handle of the created thread/process
        kiv_os::THandle handle;

        kiv_os_rtl::Clone(static_cast<kiv_os::TThread_Proc>(tthread_proc), data, handle);


        kiv_os::THandle handles[] = {handle2, handle};

        uint8_t handleThatSignalledIndex = 2;

        // wait for the program to finish
        kiv_os_rtl::Wait_For(handles, 2, handleThatSignalledIndex);

        std::wcout << "Handle that signalled index: " << std::endl;
        std::wcout << handleThatSignalledIndex << std::endl;
    } else {
        // the handle of the created thread/process
        kiv_os::THandle handle;

        // clone syscall to call a program (TThread_Proc)
        kiv_os_rtl::Clone(static_cast<kiv_os::TThread_Proc>(tthread_proc), data, handle);

        kiv_os::THandle handles[] = {handle};

        uint8_t handleThatSignalledIndex = 0;

        // wait for the program to finish
        kiv_os_rtl::Wait_For(handles, 1, handleThatSignalledIndex);
    }

}

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {
    const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    const size_t buffer_size = 256;
    char buffer[buffer_size];
    size_t counter;

    const char *intro = "Vitejte v kostre semestralni prace z KIV/OS.\n" \
                        "Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
    kiv_os_rtl::Write_File(std_out, intro, strlen(intro), counter);


    const char *prompt = "C:\\>";
    do {
        kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);

        if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter)) {
            if ((counter > 0) && (counter == buffer_size)) counter--;
            buffer[counter] = 0;    //udelame z precteneho vstup null-terminated retezec

            const char *new_line = "\n";
            kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
            //kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), counter);    //a vypiseme ho
            kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
        } else
            break;    //EOF

        // TODO improve parsing of shell commands
        char *token1;
        char *command;
        // try to separate the command name
        command = strtok_s(buffer, " ", &token1);

        // if command name was provided
        if (command) {
            // separate command arguments
            char *args = strtok_s(NULL, ",", &token1);
            if (strcmp(command, "echo") == 0) {
                if (args) {
                    call_program(echo, regs, args);
                }
            }
        }
    } while (strcmp(buffer, "exit") != 0);


    return 0;
}
