#include "shell.h"
#include "rtl.h"
#include "argparser.h"
#include <vector>
#include "error_handler.h"

constexpr auto PROMPT_BUFFER_SIZE = 5000;

void cancel_all_programs(std::vector<program> programs, std::vector<kiv_os::THandle> pipe_handles, std::vector<kiv_os::THandle> handles,
    kiv_os::THandle file_handle_first_in, kiv_os::THandle file_handle_last_out, int num_of_running, kiv_os::THandle std_in) {

    // close all the pipe handles and the file handles:
    for (int i = 0; i < pipe_handles.size(); i++) {
        kiv_os_rtl::Close_Handle(pipe_handles[i]);
    }


    if (programs[0].input.type == ProgramHandleType::File) {
        kiv_os_rtl::Close_Handle(file_handle_first_in);
    }
    else {
        // this should never be necessary!
        /*char eot = static_cast<char>(kiv_hal::NControl_Codes::EOT);
        size_t w;
        kiv_os_rtl::Write_File(std_in, &eot, 1, w);*/
    }

    if (programs[programs.size() - 1].input.type == ProgramHandleType::File) {
        kiv_os_rtl::Close_Handle(file_handle_last_out);
    }

    // now read the exit codes of the programs that were cloned:
    for (int i = num_of_running - 1; i >= 0; i--) {
        kiv_os::NOS_Error exit_code;
        kiv_os_rtl::Read_Exit_Code(handles[i], exit_code);
    }
}


void call_piped_programs(std::vector<program> programs, const kiv_hal::TRegisters &registers, std::string& nonexistProg) {

    // get references to std_in and out from respective registers:
    const auto std_out = static_cast<kiv_os::THandle>(registers.rbx.x);
    const auto std_in = static_cast<kiv_os::THandle>(registers.rax.x);

    int pipes_num = static_cast<int>(programs.size()) - 1; // we will need -1 number of pipes
    // preparing lists for the pipe handles (ins and outs) and the program handles:
    std::vector<kiv_os::THandle> pipe_handles;
    std::vector<kiv_os::THandle> handles;
    kiv_os::THandle file_handle_first_in = -1;
    kiv_os::THandle file_handle_last_out = -1;

    // create enough pipe handles and push them to the vector:
    for (int i = 0; i < pipes_num; i++) {

        kiv_os::THandle pipe[2];
        kiv_os_rtl::Create_Pipe(pipe);

        pipe_handles.push_back(pipe[0]);
        pipe_handles.push_back(pipe[1]);
    }

    // for each program assign input and output, clone the process and save it's handle:
    bool successCloning;
    int running_progs_num = 0;

    bool close_file_handle_last_out = false;

    file_handle_first_in = std_in;
    file_handle_last_out = std_out;
    // at the beginning, assign first in/last out handle:
    for (int i = 0; i < programs.size(); i++) {
        if (i == 0) {
            if (programs[i].input.type == ProgramHandleType::File) {
                kiv_os::NOS_Error error;
                if (kiv_os_rtl::Open_File(programs[i].input.name, kiv_os::NOpen_File::fmOpen_Always, 0, file_handle_first_in, error)) {
                    ;;
                }
                else {
                    if (error == kiv_os::NOS_Error::File_Not_Found) {
                        // file not found
                        char buff[200];
                        memset(buff, 0, 200);
                        size_t n = sprintf_s(buff, "File %s not found.\n", programs[i].input.name);
                        kiv_os_rtl::Write_File(std_out, buff, strlen(buff), n);
                    }
                    else {
                        handle_error_message(error, std_out);
                    }
                    return;
                }
            }
        }
        if (i == programs.size() - 1) {
            if (programs[i].output.type == ProgramHandleType::File) {
                kiv_os::NOS_Error error;
                if (kiv_os_rtl::Open_File(programs[i].output.name, static_cast<kiv_os::NOpen_File>(0), static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive), file_handle_last_out, error)) {
                    close_file_handle_last_out = true;
                }
                else {
                    //cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                    if (error == kiv_os::NOS_Error::File_Not_Found) {
                        // file not found
                        char buff[200];
                        memset(buff, 0, 200);
                        size_t n = sprintf_s(buff, "File %s not found.\n", programs[i].output.name);
                        kiv_os_rtl::Write_File(std_out, buff, strlen(buff), n);
                    }
                    else {
                        handle_error_message(error, std_out);
                    }
                    return;
                }
            }
        }
    }


    //for (int i = 0; i < programs.size(); i++) {
    for (int i = static_cast<int>(programs.size()) - 1; i >= 0; i--) {
        if (i == 0) {
            // first program gets std in or a file as input and first pipe's in as output
            kiv_os::THandle first_handle;

            if (i < programs.size() - 1) {
                successCloning = kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, file_handle_first_in/*std_in*/,
                                          pipe_handles[0], first_handle);
                if (!successCloning) {
                    cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                    nonexistProg = programs[i].name;
                    return;
                }
                else {
                    running_progs_num++;
                }

            } else if (i == programs.size() - 1) {
                successCloning = kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, file_handle_first_in/*std_in*/, file_handle_last_out/*std_out*/,
                                          first_handle);
                if (!successCloning) {
                    cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                    nonexistProg = programs[i].name;
                    return;
                }
                else {
                    running_progs_num++;
                }
            }

            handles.push_back(first_handle);

        } else if (i == programs.size() - 1) {
            // the last program gets std out or a file as output
            kiv_os::THandle last_handle;

            if (programs.size() > 1) {
                successCloning = kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, pipe_handles[pipe_handles.size() - 1],
                                          file_handle_last_out/*std_out*/, last_handle);
                if (!successCloning) {
                    cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                    nonexistProg = programs[i].name;
                    return;
                }
                else {
                    running_progs_num++;
                }
            } else {
                successCloning = kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, file_handle_first_in/*std_in*/, file_handle_last_out/*std_out*/,
                                          last_handle);
                if (!successCloning) {
                    cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                    nonexistProg = programs[i].name;
                    return;
                }
                else {
                    running_progs_num++;
                }
            }

            handles.push_back(last_handle);
        } else {
            // else connect it correctly
            kiv_os::THandle handle;
            successCloning = kiv_os_rtl::Clone_Process(programs[i].name, programs[i].data, pipe_handles[2 * static_cast<size_t>(i) - 1], pipe_handles[2 * static_cast<size_t>(i)],
                                      handle);
            if (!successCloning) {
                cancel_all_programs(programs, pipe_handles, handles, file_handle_first_in, file_handle_last_out, running_progs_num, std_in);
                nonexistProg = programs[i].name;
                return;
            }
            else {
                running_progs_num++;
            }
            handles.push_back(handle);
        }
    }

    // reverse the order of the handles, bc we cloned them last to first
    std::reverse(handles.begin(), handles.end());

    // index to the handles vector of a handle that signalled it ended:
    uint8_t handleThatSignalledIndex = 0;

    // how many programs are still running: 
    running_progs_num = static_cast<int>(programs.size());

    // copy the handles before we start ereasing them so that we know which ones to close:
    std::vector<kiv_os::THandle> orig_handles = handles;
    int num_of_handles_closed = 0;

    // while there are still some programs running:
    while (running_progs_num > 0) {
        // wait for one of them to end and when it does, read it's exit code:
        kiv_os_rtl::Wait_For(handles.data(), static_cast<int>(handles.size()), handleThatSignalledIndex);
        kiv_os::NOS_Error exit_code;
        kiv_os_rtl::Read_Exit_Code(handles[handleThatSignalledIndex], exit_code);

        // find the index of this element in the original list of handles:
        auto it = std::find(orig_handles.begin(), orig_handles.end(), handles[handleThatSignalledIndex]);
        int ind = static_cast<int>(it - orig_handles.begin()); // index to original handles coresponding to the returned index

        if (ind == 0) {

            // if it was the first program that ended, close only the input of the first pipe (at pipe_handles[0])
            if (pipe_handles.size() > 0)
                kiv_os_rtl::Close_Handle(pipe_handles[0]);
            if (programs[0].input.type == ProgramHandleType::File)
                kiv_os_rtl::Close_Handle(file_handle_first_in);

            num_of_handles_closed++;
        } else if (ind == orig_handles.size() - 1) {

            // if it was the last program that ended, close only the output of the last pipe (at pipe_handles[size - 1])
            if (pipe_handles.size() > 0) {
                kiv_os_rtl::Close_Handle(pipe_handles[pipe_handles.size() - 1]);
            }

            num_of_handles_closed++;
        } else {
            // else close the input or output of the pipes surrounding the process
            kiv_os_rtl::Close_Handle(pipe_handles[2 * static_cast<size_t>(ind)]);
            kiv_os_rtl::Close_Handle(pipe_handles[2 * static_cast<size_t>(ind) - 1]);
            num_of_handles_closed++;
            num_of_handles_closed++;
        }

        // remove the handle of the ended process from the handles array which will be sent to the waitfor function:
        handles.erase(handles.begin() + handleThatSignalledIndex);

        // decrement the number of processes running:
        running_progs_num--;
    }

    if (close_file_handle_last_out) {
        kiv_os_rtl::Close_Handle(file_handle_last_out);
    }
}

void call_program(char *program, const kiv_hal::TRegisters &registers, const char *data, kiv_os::THandle std_out) {
    // call clone from RTL
    // RTL is used we do not have to set register values here


    // the handle of the created thread/process
    kiv_os::THandle handle;

    // clone syscall to call a program (TThread_Proc)
    if (kiv_os_rtl::Clone_Process(program, data, registers.rax.x, registers.rbx.x, handle)) {

        // wait for the program to finish by attempting to read it's exit code
        kiv_os::NOS_Error exit_code;
        kiv_os_rtl::Read_Exit_Code(handle, exit_code);

        if (exit_code != kiv_os::NOS_Error::Success) {
            size_t written;
            char message_buffer[100];
            sprintf_s(message_buffer, 100, "\nProgram resulted in %u exit code.\n", exit_code);
            kiv_os_rtl::Write_File(std_out, message_buffer, strlen(message_buffer), written);
        }
    }
    else {
        size_t written;
        char message_buffer[100];
        sprintf_s(message_buffer, 100, "\nProgram '%s' not found.\n", program);
        kiv_os_rtl::Write_File(std_out, message_buffer, strlen(message_buffer), written);
        return; // unnecessary but more readable
    }
}

void fill_prompt_buffer(char *path, char *prompt_buffer, size_t prompt_buffer_size) {
    sprintf_s(prompt_buffer, prompt_buffer_size, "%s>", path);
}

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {
    bool echoOn = true;

    const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    const size_t buffer_size = 256;
    char buffer[buffer_size];
    size_t counter;

    const char *intro = "Vitejte v kostre semestralni prace z KIV/OS.\n" \
                        "Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
    kiv_os_rtl::Write_File(std_out, intro, strlen(intro), counter);

    char prompt[PROMPT_BUFFER_SIZE];
    char working_directory[PROMPT_BUFFER_SIZE];
    size_t get_wd_read_count;

    kiv_os_rtl::Get_Working_Dir(working_directory, PROMPT_BUFFER_SIZE, get_wd_read_count);

    fill_prompt_buffer(working_directory, prompt, PROMPT_BUFFER_SIZE);

    const char *new_line = "\n";
    bool eot_read = false;
    do {
        if (echoOn) {
            kiv_os_rtl::Write_File(std_out, new_line, 1, counter);
            kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);
        }

        if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, counter)) {
            if ((counter > 0) && (counter == buffer_size)) counter--;
            buffer[counter] = 0;    //udelame z precteneho vstup null-terminated retezec
        } else
            break;    //EOF

        // checking for program piping
        bool io_chain = false;

        // removing leading and tailing white spaces:
        strtrim(buffer);
        int input_len = static_cast<int>(strlen(buffer));

        for (int i = 0; i < input_len; i++) {
            if (buffer[i] == static_cast<char>(kiv_hal::NControl_Codes::EOT)) {
                eot_read = true;
                buffer[i] = '\0';
                break;
            }
            if (buffer[i] == '\n') {
                // "read a line" => this allows "echo tasklist | shell" to work
                buffer[i] = '\0';
                break;
            }
        }
        kiv_os_rtl::Write_File(std_out, "\r", 1, counter);
        if (echoOn)
            kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);
        kiv_os_rtl::Write_File(std_out, buffer, strlen(buffer), counter);    //a vypiseme ho
        kiv_os_rtl::Write_File(std_out, new_line, strlen(new_line), counter);

        for (int i = 0; i < input_len; i++) {
            if (buffer[i] == '|' ||
                buffer[i] == '>' ||
                buffer[i] == '<') {
                // possibility of io chain in the correct format
                io_chain = true;
            }
        }

        if (io_chain) {
            std::string wrongStr;
            std::vector<program> programs = parse_programs(buffer, wrongStr);
            if (!wrongStr.empty()) {
                kiv_os_rtl::Write_File(std_out, wrongStr.c_str(), wrongStr.size(), counter);
                continue;
            }

            if (programs.size() == 1 && strcmp(programs[0].name, "echo") == 0 &&
                (programs[0].input.type == ProgramHandleType::File || programs[0].output.type == ProgramHandleType::File)) {
                call_piped_programs(programs, regs, wrongStr);
                if (!wrongStr.empty()) {
                    size_t written;
                    char message_buffer[100];
                    sprintf_s(message_buffer, 100, "\nProgram '%s' not found.\n", wrongStr.c_str());
                    kiv_os_rtl::Write_File(std_out, message_buffer, strlen(message_buffer), written);
                }
                continue;
            }
            else if (!(programs.size() == 1 && strcmp(programs[0].name, "echo") == 0)) {
                // call piped programs if it's not just echo to execute
                call_piped_programs(programs, regs, wrongStr);
                if (!wrongStr.empty()) {
                    size_t written;
                    char message_buffer[100];
                    sprintf_s(message_buffer, 100, "\nProgram '%s' not found.\n", wrongStr.c_str());
                    kiv_os_rtl::Write_File(std_out, message_buffer, strlen(message_buffer), written);
                }
                continue;
            }

            // if the case WASN'T that we only had the echo program, go back to the reading loop
           /* if (!(programs.size() == 1 && strcmp(programs[0].name, "echo") == 0))
                continue;*/
        }


        std::vector<std::string> command_and_args;
        char* token1 = nullptr;
        char *p = strtok_s(buffer, " ", &token1); // make it into two tokens only
        if (p)
            command_and_args.push_back(p);
        if (token1)
            command_and_args.push_back(token1);

        if (command_and_args.empty())
            continue;


        // get the command name:
        std::string command = "";
        if (command_and_args.size() > 0)
            command = command_and_args[0];


        // if command name was provided
        if (command.size() > 0) {

            // args will be the rest after the command:
            std::string args;
            if (command_and_args.size() > 1) // just to be safe
                args = command_and_args[1];
            else args = "";

            // before executing anything else check for echo on/off...
            if (command == "echo") {
                if (args.size() > 0) {

                    if (args == "on") {
                        echoOn = true;
                        continue;
                    }
                    else if (args == "off") {
                        echoOn = false;
                        continue;
                    }
                }
            }
            else if (command == "cd") { // ...or cd
                if (!args.empty()) {
                    if (kiv_os_rtl::Set_Working_Dir(args.c_str())) {
                        kiv_os_rtl::Get_Working_Dir(working_directory, PROMPT_BUFFER_SIZE, get_wd_read_count);
                        if (get_wd_read_count > 0) {
                            fill_prompt_buffer(working_directory, prompt, PROMPT_BUFFER_SIZE);
                        } else {
                            size_t written = 0;
                            const char* error = "Could not read current working directory\n";
                            kiv_os_rtl::Write_File(std_out, error, strlen(error), written);
                        }
                    } else {
                        size_t written = 0;
                        const char* error = "Directory not found\n";
                        kiv_os_rtl::Write_File(std_out, error, strlen(error), written);
                    }
                } else {
                    size_t written = 0;
                    const char* error = "Directory not specified\n";
                    kiv_os_rtl::Write_File(std_out, error, strlen(error), written);
                }
                continue;
            }
            else if (command == "exit") {
                break;
            }
            // actual command execution:
            // cast from const to non const
            char* command_c = const_cast<char*>(command.c_str());
            // call the program:
            call_program(command_c, regs, args.c_str(), std_out);

        }
    } while (strcmp(buffer, "exit") != 0 && !eot_read);

    const char* bye_message = "\nShell exiting...\n";
    size_t written;
    kiv_os_rtl::Write_File(std_out, bye_message, strlen(bye_message), written);
    return 0;
}
