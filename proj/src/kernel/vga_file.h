//
// Created by Stanislav Král on 06.11.2020.
//

#pragma once

#include <cstddef>
#include "generic_file.h"

/**
 * Wrapper over the VGA driver
 */
class Tty_File: public Generic_File {
public:
    bool write(char *buffer, size_t size, size_t &written) override;

    bool read(size_t size, char *out_buffer, size_t &read) override;

};
