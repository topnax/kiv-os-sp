//
// Created by Stanislav Král on 06.11.2020.
//

#pragma once

#include <cstddef>
#include "generic_file.h"

/**
 * Wrapper over the VGA driver
 */
class Vga_File: public Generic_File {
public:
    size_t write(char *buffer, size_t size) override;

    size_t read(size_t size, char *out_buffer) override;

};
