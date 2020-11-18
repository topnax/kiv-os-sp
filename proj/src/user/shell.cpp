#include "shell.h"
#include "rtl.h"
#include "argparser.h"
#include <vector>


int pipe_test(kiv_os::THandle std_in, kiv_os::THandle std_out) {
    printf("pipe test :)\n");

    kiv_os::THandle pipe[2];
    kiv_os_rtl::Create_Pipe(pipe);

    kiv_os::THandle rgen_handle;
    kiv_os_rtl::Clone_Process("rgen", "", std_in, pipe[0], rgen_handle);

    kiv_os::THandle freq_handle;
    kiv_os_rtl::Clone_Process("freq", "", pipe[1], std_out, freq_handle);


    kiv_os::NOS_Error exit_code;
    kiv_os_rtl::Read_Exit_Code(rgen_handle, exit_code);
    printf("rgen stopped\n");

    kiv_os_rtl::Close_Handle(pipe[0]);

    kiv_os_rtl::Read_Exit_Code(freq_handle, exit_code);
    printf("freq stopped\n");

    kiv_os_rtl::Close_Handle(pipe[1]);
    return 0;
}

void call_program(char *program, const kiv_hal::TRegisters &registers, char *data) {
    // call clone from RTL
    // RTL is used we do not have to set register values here

    // TODO remove such test in release
    if (data && strcmp(data, "test_handles") == 0) {
        // the handle of the created thread/process
        kiv_os::THandle handle;

        // delayed echo
        kiv_os::THandle test_handle;
        kiv_os_rtl::Clone_Process(program, "delayed_echo", registers.rax.x, registers.rbx.x, test_handle);

        // clone syscall to call a program (TThread_Proc)
        kiv_os_rtl::Clone_Process(program, data, registers.rax.x, registers.rbx.x, handle);

        kiv_os::THandle handles[] = {test_handle, handle};

        uint8_t handleThatSignalledIndex = 0;
        kiv_os::NOS_Error exit_code;

        // wait for the program to finish
        kiv_os_rtl::Wait_For(handles, 2, handleThatSignalledIndex);
        kiv_os_rtl::Read_Exit_Code(handles[handleThatSignalledIndex], exit_code);
        printf("Handle that signalled first: %d, exit_code=%d\n", handleThatSignalledIndex, exit_code);

        kiv_os::THandle handles_test[] = {test_handle};
        // wait for the program to finish
        kiv_os_rtl::Wait_For(handles_test, 1, handleThatSignalledIndex);
        kiv_os_rtl::Read_Exit_Code(handles[handleThatSignalledIndex], exit_code);

        printf("Second handle that signalled: %d, exit_code=%d\n", handleThatSignalledIndex, exit_code);

    } else {
        // the handle of the created thread/process
        kiv_os::THandle handle;

        // clone syscall to call a program (TThread_Proc)
        kiv_os_rtl::Clone_Process(program, data, registers.rax.x, registers.rbx.x, handle);

        // wait for the program to finish by attempting to read it's exit code
        kiv_os::NOS_Error exit_code;
        kiv_os_rtl::Read_Exit_Code(handle, exit_code);

        if (exit_code != kiv_os::NOS_Error::Success) {
            // TODO remove in release
            printf("\nProgram resulted in %d\n", exit_code);
        }
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

        // checking for program piping - TODO make this more elegant
        bool io_chain = false;
        int input_len = strlen(buffer);
        for (int i = 0; i < input_len; i++) {
            if (buffer[i] == '|' ||
                buffer[i] == '>' ||
                buffer[i] == '<') {
                io_chain = true;
                break;
            }
        }

        if (io_chain) {
            //program* programs = (program*)malloc(10 * sizeof(program)); // todo the size will have to be resolved by argparser?
            //int* program_count = (int*)malloc(1 * sizeof(int));
            std::vector<program> programs = parse_programs2(buffer/*, programs, program_count*/);

            //for (int i = 0; i < programs.size()/**program_count*/; i++) {
            //    char* name = programs[i].name;
            //    char* data = programs[i].data;
            //    io in = programs[i].input;
            //    io out = programs[i].output;

            //    printf("%d name: '%s'\n", i, name);
            //    printf("%d data: '%s'\n", i, data);
            //    printf("%d input type: %d\n", i, in.type);
            //    printf("%d input name: '%s'\n", i, in.name);
            //    printf("%d output type: %d\n", i, out.type);
            //    printf("%d output name: '%s'\n", i, out.name);
            //    printf("\n");
            //}

            call_piped_programs(programs, regs);
            continue;
        }


        // TODO improve parsing of shell commands
        char *token1;
        char *command;
        // try to separate the command name
        command = strtok_s(buffer, " ", &token1);

        // if command name was provided
        if (command) {
            // separate command arguments
            char *args = strtok_s(NULL, " ", &token1);
            if (strcmp(command, "echo") == 0) {
                if (args) {
                    call_program("echo", regs, args);
                }
            }
            if (strcmp(command, "rgen") == 0) {
                    call_program("rgen", regs, args);
            }
            if (strcmp(command, "pipetest") == 0) {
                pipe_test(std_in, std_out);
            }
            if (strcmp(command, "freq") == 0) {
                call_program("freq", regs, args);
            }
            if (strcmp(command, "charcnt") == 0) {
                call_program("charcnt", regs, args);
            }
        }
    } while (strcmp(buffer, "exit") != 0);


    return 0;
}
