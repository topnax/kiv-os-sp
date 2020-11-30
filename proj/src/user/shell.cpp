#include "shell.h"
#include "rtl.h"
#include "argparser.h"
#include <vector>


void call_piped_programs(std::vector<program> programs, const kiv_hal::TRegisters& registers) {

    //printf("running piped programs\n");

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(registers.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(registers.rax.x);

    int pipes_num = programs.size() - 1; // we will need -1 number of pipes
    // preparing lists for the pipe handles (ins and outs) and the program handles:
    std::vector<kiv_os::THandle> pipe_handles;
    std::vector<kiv_os::THandle> handles;
    kiv_os::THandle file_handle_first_in;
    kiv_os::THandle file_handle_last_out;

    // create enough pipe handles and push them to the vector:
    for (int i = 0; i < pipes_num; i++) {

        kiv_os::THandle pipe[2];
        kiv_os_rtl::Create_Pipe(pipe);

        pipe_handles.push_back(pipe[0]);
        pipe_handles.push_back(pipe[1]);
    }

    // for each program assign input and output, clone the process and save it's handle:
    for (int i = 0; i < programs.size(); i++) {
        if (i == 0) {
            // first program gets std in or a file as input and first pipe's in as output
            kiv_os::THandle first_handle;
            
            if (programs[i].input.type == ProgramHandleType::File) {
                if (kiv_os_rtl::Open_File(programs[i].input.name, 0, 0, file_handle_first_in)) {
                    ;;
                }
                else {
                    // file not found?
                    char buff[200];
                    memset(buff, 0, 200);
                    size_t n = sprintf_s(buff, "File %s not found.", programs[i].input.name);
                    kiv_os_rtl::Write_File(std_out, buff, strlen(buff), n);
                    return;
                }
            }
            else if (programs[i].input.type == ProgramHandleType::Standard) {
                file_handle_first_in = std_in;
            }
            
            if (i < programs.size() - 1) {
                kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, file_handle_first_in/*std_in*/, pipe_handles[0], first_handle);
            }
            else if (i == programs.size() - 1) {
                kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, file_handle_first_in/*std_in*/, std_out, first_handle);
            }
            
            handles.push_back(first_handle);

        }
        else if (i == programs.size() - 1) { // TODO test output to file!
            // the last program gets std out or a file as output
            kiv_os::THandle last_handle;

            if (programs[i].output.type == ProgramHandleType::File) {
                if (kiv_os_rtl::Open_File(programs[i].output.name, 0, 0, file_handle_last_out)) {
                    ;;
                }
                else {
                    // file not found?
                    char buff[200];
                    memset(buff, 0, 200);
                    size_t n = sprintf_s(buff, "File %s not found.", programs[i].output.name);
                    kiv_os_rtl::Write_File(std_out, buff, strlen(buff), n);
                    return;
                }
            }
            else if (programs[i].output.type == ProgramHandleType::Standard) {
                file_handle_last_out = std_out;
            }

            if (programs.size() > 1) {
                kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, pipe_handles[pipe_handles.size() - 1], file_handle_last_out/*std_out*/, last_handle);
            }
            else {
                kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, std_in, file_handle_last_out/*std_out*/, last_handle);
            }
                
            handles.push_back(last_handle);
        }
        else { // TODO this should be fine
            // else connect it correctly
            kiv_os::THandle handle;
            kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, pipe_handles[2 * i - 1], pipe_handles[2 * i], handle);
            handles.push_back(handle);
        }
    }

    // index to the handles vector of a handle that signalled it ended:
    uint8_t handleThatSignalledIndex = 0;
    
    // how many programs are still running: 
    int running_progs_num = programs.size();

    // copy the handles before we start ereasing them so that we know which ones to close:
    std::vector<kiv_os::THandle> orig_handles = handles;
    int num_of_handles_closed = 0; // debug variable - TODO remove in release

    // while there are still some programs running:
    while (running_progs_num > 0) {
        // wait for one of them to end and when it does, read it's exit code:
        kiv_os_rtl::Wait_For(handles.data(), handles.size(), handleThatSignalledIndex);
        kiv_os::NOS_Error exit_code;
        kiv_os_rtl::Read_Exit_Code(handles[handleThatSignalledIndex], exit_code);
        //printf("Exit code for handle %d = %d\n", handles[handleThatSignalledIndex], exit_code);

        // find the index of this element in the original list of handles:
        auto it = std::find(orig_handles.begin(), orig_handles.end(), handles[handleThatSignalledIndex]);
        int ind = it - orig_handles.begin(); // index to original handles coresponding to the returned index

        if (ind == 0) {
            //printf("closing handle %d (ind %d)\n", pipe_handles[0], 0);

            // if it was the first program that ended, close only the input of the first pipe (at pipe_handles[0])
            if(pipe_handles.size() > 0)
                kiv_os_rtl::Close_Handle(pipe_handles[0]);
            if(programs[0].input.type == ProgramHandleType::File)
                kiv_os_rtl::Close_Handle(file_handle_first_in);

            num_of_handles_closed++;
        }
        else if (ind == orig_handles.size() - 1) {
            //printf("closing handle %d (ind %d)\n", pipe_handles[pipe_handles.size() - 1], pipe_handles.size() - 1);

            // if it was the last program that ended, close only the output of the last pipe (at pipe_handles[size - 1])
            if (pipe_handles.size() > 0)
                kiv_os_rtl::Close_Handle(pipe_handles[pipe_handles.size() - 1]);
            if (programs[programs.size() - 1].input.type == ProgramHandleType::File)
                kiv_os_rtl::Close_Handle(file_handle_last_out);

            num_of_handles_closed++;
        }
        else {
            //printf("closing handles %d (ind %d) and %d (ind %d)\n", pipe_handles[2 * ind], 2 * ind, pipe_handles[2 * ind - 1], 2 * ind - 1);

            // else close the input or output of the pipes surounding the process
            kiv_os_rtl::Close_Handle(pipe_handles[2 * ind]);
            kiv_os_rtl::Close_Handle(pipe_handles[2 * ind - 1]);
            num_of_handles_closed++;
            num_of_handles_closed++;
        }

        // remove the handle of the ended process from the handles array which will be sent to the waitfor function:
        handles.erase(handles.begin() + handleThatSignalledIndex);

        // decrement the number of processes running:
        running_progs_num--;
    }
}

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

void call_program(char *program, const kiv_hal::TRegisters &registers, const char *data) {
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

void fill_supported_commands_set(std::set<std::string>& set) {
    // hardcoded filling of supported commands:
    set.insert("type");
    set.insert("md");
    set.insert("rd");
    set.insert("dir");
    set.insert("echo");
    set.insert("find");
    set.insert("sort");
    set.insert("rgen");
    set.insert("freq");
    set.insert("tasklist");
    set.insert("shutdown");
    set.insert("charcnt");
}

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {
    std::set<std::string> supported_commands;
    fill_supported_commands_set(supported_commands);

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

        // TODO proper PROPER format checking?
        // - current state allows this to happen:
        // user types 'echo hello | freq |' -> last pipe symbol doesn't allow piped programs to be called,
        // the result is 'hello' printed to the console - OK?

        // checking for program piping - TODO make this more elegant
        bool io_chain = false;
        
        // removing leading and tailing white spaces:
        strtrim(buffer);
        int input_len = strlen(buffer);

        for (int i = 0; i < input_len; i++) {
            if (buffer[i] == '|' ||
                buffer[i] == '>' ||
                buffer[i] == '<') {
                // possibility of io chain in the correct format

                if (buffer[i] == '|' &&
                    (i == input_len - 1 || i == 0) ) {
                    // the pipe symbol was at the beginning with nothing before it
                    // or it was at the end with nothing behind it
                    //
                    // in such case do not set the io_chain flag to true - the later implementation of piped 
                    // program calling is not prepared for this situation
                    io_chain = false;
                    break; // this break might be unnecessary
                }
                else {
                    io_chain = true;
                }
            }
        }

        if (io_chain) {
            std::vector<program> programs = parse_programs(buffer);

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

            // checking if all the program names are valid:
            bool names_ok = false;
            for (int i = 0; i < programs.size(); i++) {
                names_ok = supported_commands.find(programs[i].name) != supported_commands.end();
                if (!names_ok) {
                    char msg[100];
                    size_t written;
                    sprintf_s(msg, 100, "Program \"%s\" not found\n", programs[i].name);
                    kiv_os_rtl::Write_File(std_out, msg, strlen(msg), written);
                    break;
                }
            }

            if (names_ok) {
                call_piped_programs(programs, regs);
            }
            
            // after all is done, go back to the reading loop, do not go ahead to 'normal' commands execution
            continue;
        }


        // TODO improve parsing of shell commands
        char *token1;
        //char *command;
        std::vector<std::string> command_and_args;
        char* p = strtok_s(buffer, " \"", &token1); // make it into two tokns only
        if(p)
            command_and_args.push_back(p);
        if(token1)
            command_and_args.push_back(token1);

        if (command_and_args.empty())
            continue;


        // get the command name:
        std::string command = command_and_args[0];


        // if command name was provided and it is valid
        if (command.size() > 0 && 
            supported_commands.find(command) != supported_commands.end()) {

            // args will be the rest after the command:
            std::string args = command_and_args[1];

            if (command == "echo") {
                if (args.size() > 0) {
                    if (args[0] == '\"' && args[args.size() - 1] == '\"') { // escaping the double quotes, maybe this is not necessary
                        args = args.substr(1, args.size() - 2);
                    }
                    call_program("echo", regs, args.c_str());
                }
            }
            else if (command == "rgen") {
                    call_program("rgen", regs, args.c_str());
            }
            else if (command == "pipetest") {
                pipe_test(std_in, std_out);
            }
            else if (command == "freq") {
                call_program("freq", regs, args.c_str());
            }
            else if (command == "charcnt") {
                call_program("charcnt", regs, args.c_str());
            }
            else if (command == "tasklist") {
                call_program("tasklist", regs, args.c_str());
            }
            else if (command == "sort") {
                args = trim(args, " ");
                call_program("sort", regs, args.c_str());
            }
            else if (command == "find") {

                // the format is find /v /c "" file.txt
                std::string form = "/v /c \"\"";
                int pos = 0;
                int index = args.find(form, pos);
                if (index == 0) {
                    // the format is ok, now separate the file path:
                    args = args.substr(form.size(), args.size() - 1);
                    args = trim(args, " ");
                    call_program("find", regs, args.c_str());
                }
            }
            else if (command == "dir") {
                // TODO improve arg parsing
                if (args.size() > 0) {
                    call_program("dir", regs, args.c_str());
                }
            }
            else if (command == "type") {
                // TODO improve argparsing
                if (args.size() > 0) {
                    call_program("type", regs, args.c_str());
                }
            }
            // TODO this command might not be present in the release
            else if (command == "shutdown") {
                kiv_os_rtl::Shutdown();
            }
        }
    } while (strcmp(buffer, "exit") != 0);


    return 0;
}
