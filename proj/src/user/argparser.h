#pragma once
//
// Created by Stanislav Král on 26.10.2020.
//


#include <cstdint>
#include <vector>

const size_t NAME_LEN = 200;
const size_t DATA_LEN = 200;

enum class ProgramHandleType : std::uint8_t {
    Standard = 1,
    Pipe,
    File
};

struct io
{
    ProgramHandleType type;
    char name[NAME_LEN]; // name of the program / file

};

struct program {
    char name[NAME_LEN];
    char data[DATA_LEN]; // todo maybe data should be allocated dynamically, bc it can be long?
    //ProgramHandleType input;
    //ProgramHandleType output;
    io input;
    io output;
};

void parse_programs(char *input, program *programs, int *length);
std::vector<program> parse_programs2(char* input/*, program* programs, int* length*/);