//
// Created by Stanislav Král on 06.11.2020.
//

#pragma once

#include <cstddef>
#include "generic_file.h"
#include "pipe.h"
#include <memory>

class Pipe_In_File: public Generic_File {
public:

    // takes a shared_ptr of a pipe (one pipe is shared between two file objects IN/OUT)
    // the pipe should be deleted once both file objects are destroyed
    explicit Pipe_In_File(std::shared_ptr<Pipe> pipe);

    kiv_os::NOS_Error write(char *buffer, size_t size, size_t &written) override;

    kiv_os::NOS_Error read(size_t size, char *out_buffer, size_t &read) override {
        return false;
    };

    kiv_os::NOS_Error seek(size_t value, kiv_os::NFile_Seek position, kiv_os::NFile_Seek op, size_t &res) override;

    void close() override;

private:
    std::shared_ptr<Pipe> pipe;
};

class Pipe_Out_File: public Generic_File {
public:

    // takes a shared_ptr of a pipe (one pipe is shared between two file objects IN/OUT)
    // the pipe should be deleted once both file objects are destroyed
    explicit Pipe_Out_File(std::shared_ptr<Pipe>);

    kiv_os::NOS_Error write(char *buffer, size_t size, size_t &written) override {
        return false;
    };

    kiv_os::NOS_Error read(size_t size, char *out_buffer, size_t &read) override;

    void close() override;

private:
    std::shared_ptr<Pipe> pipe;
};
