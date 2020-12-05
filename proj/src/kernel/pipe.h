#pragma once

#include <cstdint>
#include <vector>
#include "semaphore.h"
#include "../api/api.h"

class Pipe {
private:
    // flag indicating whether the IN handle has been closed
    bool closed_in = false;

    // flag indicating whether the IN handle has been closed
    bool closed_out = false;

    // write/read pointers
    size_t write_ptr = 0;
    size_t read_ptr = 0;

    // the capacity of the pipe in chars (how many characters can this pipe hold)
    size_t capacity;

    // char buffer
    std::vector<char> buffer;

    // see producer/consumer definition
    Semaphore empty_count;
    Semaphore fill_count;

    bool write_char(char c);

public:
    explicit Pipe(size_t capacity);

    std::vector<char> Read(size_t nodes_to_read, bool &nothing_left_to_read);

    size_t Write(std::vector<char> data);

    void Close_In();

    void Close_Out();

};