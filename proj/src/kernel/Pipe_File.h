//
// Created by Stanislav Král on 06.11.2020.
//

#pragma once

#include <cstddef>
#include "Generic_File.h"
#include "pipe.h"
#include <memory>

class Pipe_In_File: public Generic_File {
public:

    // takes a shared_ptr of a pipe (one pipe is shared between two file objects IN/OUT)
    // the pipe should be deleted once both file objects are destroyed
    explicit Pipe_In_File(std::shared_ptr<Pipe> pipe);

    size_t write(char *buffer, size_t size) override;

    size_t read(size_t size, char *out_buffer) override {
        return -1;
    };

    void close() override;

private:
    std::shared_ptr<Pipe> pipe;
};


class Pipe_Out_File: public Generic_File {
public:

    // takes a shared_ptr of a pipe (one pipe is shared between two file objects IN/OUT)
    // the pipe should be deleted once both file objects are destroyed
    explicit Pipe_Out_File(std::shared_ptr<Pipe>);

    size_t write(char *buffer, size_t size) override {
        return -1;
    };

    size_t read(size_t size, char *out_buffer) override;

    void close() override;

private:
    std::shared_ptr<Pipe> pipe;
};
