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
    virtual bool write(char *buffer, size_t size, size_t &written) = 0;

    // reads `size` bytes from this file to the `out_buffer`
    virtual bool read(size_t size, char *out_buffer, size_t &read) = 0;

    // closes this file
    virtual void close() { }

    // virtual destructor required, so that deleting child class instances invokes child destructor
    virtual ~Generic_File() = default;
};
