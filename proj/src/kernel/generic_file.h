//
// Created by Stanislav Král on 06.11.2020.
//
#pragma once

/**
 * An abstract class defining methods of a file object
 */
class Generic_File {
public:

    // writes `size` bytes from `buffer` to this file
    virtual size_t write(char *buffer, size_t size) = 0;

    // reads `size` bytes from this file to the `out_buffer`
    virtual size_t read(size_t size, char *out_buffer) = 0;

    // closes this file
    virtual void close() { }

    // virtual destructor required, so that deleting child class instances invokes child destructor
    virtual ~Generic_File() = default;
};
