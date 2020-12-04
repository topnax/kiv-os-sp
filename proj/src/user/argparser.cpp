//
// Created by Stanislav Král on 26.10.2020.
//

#include <cstdio>
#include <cstring>
#include <vector>
#include "rtl.h"
#include "argparser.h"
#include <string>

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

std::string trim(const std::string& str, const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}



void parse_programs_unused(char *input, program *programs, int *length) {

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


std::vector<program> parse_programs(char* input) {

    std::vector<program> programs_vec;
    const char in_symb = '<';
    const char out_symb = '>';
    const char pipe_symb = '|';
    std::vector<char> delims;

    size_t len = strlen(input);

    char curr_prog_name[NAME_LEN];
    memset(curr_prog_name, 0, NAME_LEN);

    int j = 0; // pointer to buffer containing one program's name (and potentially data)
    int prog_count = 0; // number of programs but also files in the whole pipeline
    int actual_prog_count = 0; // number of programs alone in the pipeline
    bool echoQuotes = false;

    // this cycle goes through the user input and saves names and data of programs and names of files
    for (int i = 0; i < len; ++i) {

        
        // if there is a delimiter or the end of input:
        if (input[i] == pipe_symb || 
            input[i] == in_symb || 
            input[i] == out_symb ||
            i == len - 1
            /*(echoQuotes && input[i] == '\"')*/) {

            // create new program struct
            program curr_p = program{};
            io in = io{};
            io out = io{};
            curr_p.input = in;
            curr_p.output = out;


            if (i == len - 1) {
                // add the last character if we're at the end
                curr_prog_name[j] = input[i];
                curr_prog_name[j + 1] = '\0';
            }

            
            // check if there're any arguments:
            //std::string savedStr = curr_prog_name;
            char* data;
            std::string saved = curr_prog_name;
            char* actual_name = strtok_s(curr_prog_name, " ", &data); // actual_name is the name of the program

            
            // special case for echo, bc it can have any characters in double quotes - ignore these characters, only send them to args:
            // todo think of prettier way to do this?
            //if (strcmp(actual_name, "echo") == 0 /*&& data && data[0] == '\"' && !echoQuotes*/) {

            //    // they want to echo something in quotes
            //    echoQuotes = true;

            //    // get the whole data:
            //    int k = i;
            //    for (k = i; k < len && input[k] != '\"'; k++) {
            //        saved += input[k];
            //    }
            //    if(k < len)
            //        saved += input[k];

            //    //printf("data is now %s\n", saved.c_str());

            //    memset(curr_prog_name, 0, NAME_LEN); 
            //    // copy it back to the variable we use for everything else:
            //    strncpy_s(curr_prog_name, saved.c_str(), NAME_LEN);
            //    i = k; // set the positions properly
            //    j = saved.size();

            //    if(i < len - 1)
            //        continue; // if we're not at the end yet, keep reading
            //    else {
            //        // if we are at the end, parse it again and move on
            //        actual_name = strtok_s(curr_prog_name, " ", &data); 
            //    }
            //}
            

            // trim the name and the data
            strtrim(actual_name);
            strtrim(data);
            
            // copy the name and the data into the structure
            strncpy_s(curr_p.name, actual_name, NAME_LEN);
            strncpy_s(curr_p.data, data, DATA_LEN);

            
            if (i != len - 1) {
                delims.push_back(input[i]); // save the delimiter, we'll need it later for io assigning
            }
            
            
            programs_vec.push_back(curr_p); // save the program to the vector
            prog_count++;
            actual_prog_count++;
            if (input[i] == in_symb || input[i] == out_symb) {
                actual_prog_count--; // keeping track of how many runnable programs we have
            }

            j = 0; // reset the pointer to curr_name
            i++; // skip this char, bc it's the delimiter
            memset(curr_prog_name, 0, NAME_LEN); // null the name
            echoQuotes = false; // reset the quotes flag
        }

        curr_prog_name[j] = input[i];
        curr_prog_name[j + 1] = '\0';
        j++;


        // check if we got echo and if so, take care of it:
        if (strcmp(curr_prog_name, "echo ") == 0) {
            printf("echoocococo\n");

            // there is echo. Check the following input and look for double quotes
            std::string echoArg;
            std::vector<int> quotesIndexes;
            bool quotesOpen = false;

            // read until we encounter a pipe/fwd symbol, which is not closed in double quotes:
            for (int k = i + 1; k < len; k++) {
                if (input[k] == '\"') {
                    quotesOpen = !quotesOpen;
                }
                if ((input[k] == pipe_symb || input[k] == in_symb || input[k] == out_symb) &&
                    !quotesOpen) {

                    // add the echo program with the whole arg and move on!
                    echoArg = "echo " + echoArg;
                    strcpy_s(curr_prog_name, echoArg.c_str());
                    i = k - 1;
                    j = echoArg.size();
                    break;

                }

                echoArg += input[k];

                if (k == len - 1) {
                    echoArg = "echo " + echoArg;
                    strcpy_s(curr_prog_name, echoArg.c_str());
                    i = k - 1;
                    j = echoArg.size();
                    break;
                }
            }

        }
    }




    // go through the programs and assign correct inputs and outputs to them:
    for (int i = 0; i < prog_count - 1; ++i) {

        memset(programs_vec[i].input.name, 0, NAME_LEN);
        memset(programs_vec[i].output.name, 0, NAME_LEN);
        
        if (i == 0) {
            // first program, because they are saved in the order they appered
            // assign std in to the first program and std out to the last program - this may change as we go through the pipeline
            programs_vec[i].input.type = ProgramHandleType::Standard;
            programs_vec[actual_prog_count - 1].output.type = ProgramHandleType::Standard;

        }
        else {
            // assign the input of the current program as the output of the previous one
            programs_vec[i].input.type = programs_vec[i - 1].output.type;
            strncpy_s(programs_vec[i].input.name, programs_vec[i - 1].name, NAME_LEN);
        }

        if (delims[i] == pipe_symb) {
            // if there is a | symbol between current program and the next one:
            // set the output type of the current program to pipe and to which program it connects (the next one)
            // the same is done for the input of the next program
            programs_vec[i].output.type = ProgramHandleType::Pipe;
            strncpy_s(programs_vec[i].output.name, programs_vec[i + 1].name, NAME_LEN);
            
            programs_vec[i + 1].input.type = ProgramHandleType::Pipe;
            strncpy_s(programs_vec[i + 1].input.name, programs_vec[i].name, NAME_LEN);
        }
        else if (delims[i] == out_symb) {
            // if there is > symbol between current program and the next one:
            // set the output type of the current program to file and the name of the file
            programs_vec[i].output.type = ProgramHandleType::File;
            strncpy_s(programs_vec[i].output.name, programs_vec[i + 1].name, NAME_LEN);

            //new_prog_count--; // todo destory the structs already created even for the files?
        }
        else if (delims[i] == in_symb) {
            // if there is < symbol between current program and the next one:
            // set the input type of the first program to file and the name of the file
            programs_vec[0].input.type = ProgramHandleType::File;
            strncpy_s(programs_vec[0].input.name, programs_vec[i + 1].name, NAME_LEN);

            //new_prog_count--;
        }

    }

    // remove the files, if there were any they must be at the end:
    programs_vec.resize(actual_prog_count); 
    return programs_vec;
}

