//
// Created by Stanislav Král on 26.10.2020.
//

#include <cstdio>
#include <cstring>
#include <vector>
#include "argparser.h"

// TODO basic argparser implementation

void strtrim(char *str) {
    int start = 0; // number of leading spaces
    char *buffer = str;
    while (*str && *str++ == ' ') ++start;
    while (*str++); // move to end of string
    int end = str - buffer - 1;
    while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
    buffer[end] = 0; // remove trailing spaces
    if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
    str = buffer + start;
    while ((*buffer++ = *str++));  // remove leading spaces: K&R
}

void parse_programs(char *input, program *programs, int *length) {

    auto last_input = ProgramHandleType::Standard;
    auto last_output = ProgramHandleType::Standard;
    size_t program_count = 0;

    size_t p_len = 0;
    size_t len = strlen(input);
    for (int i = 0; i < len; ++i) {
        if (input[i] == '|' || input[i] == '>' || i == len - 1) {
            if (p_len > 1) {
                if (i < len - 1) {
                    last_output = input[i] == '|' ? ProgramHandleType::Pipe : ProgramHandleType::File;
                } else {
                    last_output = ProgramHandleType::Standard;
                }
                program p = program{};
                //p.input = last_input;
                //p.output = last_output;
                size_t  index = p_len + (i == len - 1 ? 1 : 0);
                strncpy_s(reinterpret_cast<char *>(p.name), 200, (char *) input + i - p_len,
                          index);
                p.name[index] = '\0';
                strtrim(p.name);
                last_input = last_output;
                programs[program_count] = p;
                program_count++;
                p_len = 0;
            }
        } else {
            p_len++;
        }

    }

    *length = program_count;

}


std::vector<program> parse_programs2(char* input/*, program* programs, int* length*/) {

    std::vector<program> programs_vec;
    const char in_symb = '<';
    const char out_symb = '>';
    const char pipe_symb = '|';
    std::vector<char> delims;

    size_t len = strlen(input);

    char curr_prog_name[NAME_LEN];
    memset(curr_prog_name, 0, NAME_LEN);

    bool in_found = false;
    bool out_found = false;
    int j = 0;
    int prog_count = 0;
    int actual_prog_count = 0;
    for (int i = 0; i < len; ++i) {

        
        // if there is a delimiter or the end of input:
        if (input[i] == pipe_symb || 
            input[i] == in_symb || 
            input[i] == out_symb ||
            i == len - 1) { 

            // create new program struct
            program curr_p = program{};
            io in = io{};
            io out = io{};
            curr_p.input = in;
            curr_p.output = out;


            // --- TAKING CARE OF NAME AND DATA: ---
            if (i == len - 1) {
                // add the last character if we're at the end
                curr_prog_name[j] = input[i];
            }

            

            
            // check if there's data:
            char* data;
            char* actual_name = strtok_s(curr_prog_name, " ", &data);

            // trim the name and the data
            strtrim(actual_name);
            strtrim(data);
            
            // copy the name and the data in
            strncpy_s(curr_p.name, actual_name, NAME_LEN);
            strncpy_s(curr_p.data, data, DATA_LEN);
            // --- ---


            
            if (i != len - 1) {
                delims.push_back(input[i]); // save the delimiter, we'll need it later for io assigning
            }
            
            j = 0; // reset the pointer to curr_name
            i++; // skip this char, bc it's the delimiter

            //programs[prog_count] = curr_p;
            programs_vec.push_back(curr_p);
            prog_count++;
            actual_prog_count++;
            if (input[i - 1] == in_symb || input[i - 1] == out_symb) {
                actual_prog_count--;
            }

            memset(curr_prog_name, 0, NAME_LEN);
        }

        curr_prog_name[j] = input[i];
        j++;
    }




    // go through the programs and assign correct inputs and outputs to them
    int delims_count = delims.size(); // delims.size should be equal to prog_count - 1
    int del = 0;
    size_t new_prog_count = prog_count;

    // preparing such flow at the beginning - can be changed later in the cycle
    //programs[0].input.type = ProgramHandleType::Standard;
    //programs[prog_count - 1].output.type = ProgramHandleType::Standard;

    for (int i = 0; i < prog_count - 1; ++i) {

        // ---  TAKING CARE OF I/O ---
        /*io curr_in = io{};
        io curr_out = io{};*/

        memset(programs_vec[i].input.name, 0, NAME_LEN);
        memset(programs_vec[i].output.name, 0, NAME_LEN);

        //programs[i].input.type = programs[i - 1].output.type;
        //strncpy_s(programs[i].input.name, programs[i - 1].name, NAME_LEN);
        
        if (i == 0) {
            // first program, because they are saved in the order they appered
            programs_vec[i].input.type = ProgramHandleType::Standard;
            programs_vec[actual_prog_count - 1].output.type = ProgramHandleType::Standard;

        }
        else {
            
            programs_vec[i].input.type = programs_vec[i - 1].output.type;
            strncpy_s(programs_vec[i].input.name, programs_vec[i - 1].name, NAME_LEN);
            /*if(del < delims_count - 1)
                del++;*/
        }

        if (delims[i] == pipe_symb) {
            programs_vec[i].output.type = ProgramHandleType::Pipe;
            strncpy_s(programs_vec[i].output.name, programs_vec[i + 1].name, NAME_LEN);
            
            programs_vec[i + 1].input.type = ProgramHandleType::Pipe;
            strncpy_s(programs_vec[i + 1].input.name, programs_vec[i].name, NAME_LEN);
        }
        else if (delims[i] == out_symb) {
            programs_vec[i].output.type = ProgramHandleType::File;
            strncpy_s(programs_vec[i].output.name, programs_vec[i + 1].name, NAME_LEN);

            new_prog_count--; // todo destory the structs already created even for the files?
        }
        else if (delims[i] == in_symb) {
            programs_vec[0].input.type = ProgramHandleType::File;
            strncpy_s(programs_vec[0].input.name, programs_vec[i + 1].name, NAME_LEN);

            new_prog_count--;
        }

    }

    /*programs = (program*)malloc(new_prog_count * sizeof(program));
    for (int i = 0; i < new_prog_count; i++) {
        programs[i] = programs_vec[i];
    }*/



    //programs = programs_vec.data(); // &programs_vec[0];
    //*length = new_prog_count;//programs_vec.size();

    programs_vec.resize(new_prog_count);

    return programs_vec;
}

