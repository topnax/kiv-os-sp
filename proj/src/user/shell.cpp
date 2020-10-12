#include <iostream>
#include "shell.h"
#include "rtl.h"

void call_program(size_t (__stdcall echo)(const kiv_hal::TRegisters &regs), const kiv_hal::TRegisters &pRegisters) {
    std::cout << "calling a fx" << std::endl;

    kiv_os_rtl::Clone();

    echo(pRegisters);
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
            kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), counter);    //a vypiseme ho
            kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);
        } else
            break;    //EOF

        if (strcmp(buffer, "echo") == 0) {
            call_program(echo, regs);
        }
    } while (strcmp(buffer, "exit") != 0);


    return 0;
}
