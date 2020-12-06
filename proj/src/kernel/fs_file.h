//
// Created by Stanislav Král on 25.11.2020.
//

#pragma once

#include "generic_file.h"
#include "vfs.h"

class Filesystem_File : public Generic_File {
public:

    explicit Filesystem_File(VFS *vfs, File file);

    kiv_os::NOS_Error write(char *buffer, size_t size, size_t &written) override;

    kiv_os::NOS_Error read(size_t size, char *out_buffer, size_t &read) override;

    kiv_os::NOS_Error seek(size_t value, kiv_os::NFile_Seek position, kiv_os::NFile_Seek op, size_t &res) override;

    void close() override;

    ~Filesystem_File() override;

private:
    /**
     * A structure that holds the state of the file
     */
    File file;

    /**
     * Reference to the VFS this file exists in
     */
    VFS *vfs;

    bool has_attribute(kiv_os::NFile_Attributes attribute) const;

    bool is_read_only();
    bool is_system_file();
    bool is_directory();
    bool is_hidden();
    bool is_volume_id();
    bool is_archive();
};
