//
// Created by Stanislav Král on 06.11.2020.
//

#pragma once

#include <cstddef>
#include "vfs.h"
#include "fs_file.h"
#include "process/process_entry.h"


/**
 * Implements the FAT12 filesystem
 */
class Fat_Fs: public VFS {

public:

    explicit Fat_Fs(uint8_t disk_number, kiv_hal::TDrive_Parameters parameters);

    void init();

    // to override

    kiv_os::NOS_Error read(File file, size_t size, size_t offset, std::vector<char> &out) override;

    kiv_os::NOS_Error readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) override;

    kiv_os::NOS_Error open(const char *name, uint8_t flags, uint8_t attributes, File &file) override;

    kiv_os::NOS_Error mkdir(const char *name) override;

    kiv_os::NOS_Error rmdir(const char *name) override;

    kiv_os::NOS_Error write(File file, std::vector<char> buffer, size_t size, size_t offset, size_t &written) override;

    kiv_os::NOS_Error unlink(const char *name) override;

    kiv_os::NOS_Error close(File file) override;

    bool file_exists(int32_t current_fd, const char* name, bool start_from_root, bool is_folder, int32_t& found_fd) override;

private:
    uint8_t disk_number;
    kiv_hal::TDrive_Parameters disk_parameters;
};
