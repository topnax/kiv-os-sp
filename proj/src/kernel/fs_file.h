//
// Created by Stanislav Král on 25.11.2020.
//

#pragma once

#include "generic_file.h"
#include "vfs.h"

class Filesystem_File : public Generic_File {
public:

    explicit Filesystem_File(VFS *vfs, File file);

    bool write(char *buffer, size_t size, size_t &written) override;

    bool read(size_t size, char *out_buffer, size_t &read) override;

    kiv_os::NOS_Error seek(size_t value, kiv_os::NFile_Seek position, kiv_os::NFile_Seek op, size_t &res) override;

    void close() override;

private:
    /**
     * A structure that holds the state of the file
     */
    File file;

    /**
     * Reference to the VFS this file exists in
     */
    VFS *vfs;
};
