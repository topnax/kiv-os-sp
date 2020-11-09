//
// Created by Stanislav Král on 26.10.2020.
//

#include <cstdio>
#include <cstring>
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
                p.input = last_input;
                p.output = last_output;
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
