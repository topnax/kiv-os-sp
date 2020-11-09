#pragma once
//
// Created by Stanislav Král on 26.10.2020.
//


#include <cstdint>

enum class ProgramHandleType : std::uint8_t {
    Standard = 1,
    Pipe,
    File
};

struct program {
    char name[200];
    ProgramHandleType input;
    ProgramHandleType output;
};

void parse_programs(char *input, program *programs, int *length);