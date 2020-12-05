//
// Created by Stanislav Král on 24.11.2020.
//

#pragma once

#include <vector>
#include "../api/api.h"
#include <string>

struct File {
    kiv_os::THandle handle;
    uint8_t attributes;
    size_t size;
    size_t position;
    char *name;
};

/**
 * An abstract class defining a filesystem API
 */
class VFS {
public:
    /**
     * Creates the directory with the given name
     */
    virtual kiv_os::NOS_Error mkdir(const char *name) = 0;

    /**
     * Removes the directory with the given name
     */
    virtual kiv_os::NOS_Error rmdir(const char *name) = 0;

    /**
     * Reads the contents of the given directory, storing all entries into the vector passed in the `entries` parameter
     */
    virtual kiv_os::NOS_Error readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) = 0;

    /**
     * Tries to open the file with the given name, flags and attributes. The information about the file is eventually stored in the `file` reference
     */
    virtual kiv_os::NOS_Error open(const char *name, kiv_os::NOpen_File flags, uint8_t attributes, File &file) = 0;

    /**
     * Writes `size` bytes from `buffer` to the given file using the provided offset.
     * The amount of bytes written is stored in the `written` reference.
     */
    virtual kiv_os::NOS_Error write(File file, std::vector<char> buffer, size_t size, size_t offset, size_t &written) = 0;

    /**
     * Reads `size` bytes from this file to the `out` vector while using the provided offset.
     */
    virtual kiv_os::NOS_Error read(File file, size_t size, size_t offset, std::vector<char> &out) = 0;

    /**
     * Deletes the file with the given name
     */
    virtual kiv_os::NOS_Error unlink(const char *name) = 0;

    /**
     * Checks whether an item with the given `name` exists in the directory specified by `current_fd` identifier.
     * When `start_from_root` is set to `true` then `current_fd` should be ignored and `name` should be searched for in
     * the root directory of the filesystem.
     *
     * @param current_fd specifies in which directory the file should be searched for
     * @param name name of the file to be searched for
     * @param start_from_root flag specifying whether the `current_fd` should be ignored and the search performed from the root of the fs
     * @param found_fd reference to an int variable in which the identifier of the file should be stored if found
     *
     * @return `true` when the an item with the given name exists in the particular directory, otherwise `false`
     */
    virtual bool file_exists(int32_t current_fd, const char *name, bool start_from_root, int32_t &found_fd) = 0;

    /**
     * Sets the attributes of the file specified by the ˙name˙
     * @param name name of the file (including the path)
     * @param attributes attributes to be set (see kiv_os::NFile_Attributes)
     * @return result code
     */
    virtual kiv_os::NOS_Error set_attributes(const char *name, uint8_t attributes) {
        // TODO implement
        return kiv_os::NOS_Error::Permission_Denied;
    };

    /**
     * Gets the attributes of the file specified by the ˙name˙
     * @param name name of the file (including the path)
     * @param out_attributes reference to uint8_t in which the attributes should be stored
     * @return result code
     */
    virtual kiv_os::NOS_Error get_attributes(const char *name, uint8_t &out_attributes) {
        // TODO implement
        return kiv_os::NOS_Error::Permission_Denied;
    };

    /**
     *  Closes the given file
     */
    virtual kiv_os::NOS_Error close(File file) = 0;

    virtual void print_name() = 0;

    /**
     * Prints the directory entries into a vector of characters
     */
    static std::vector<char> generate_dir_vector(const std::vector<kiv_os::TDir_Entry> &entries) {
        // prepare out buffer
        std::vector<char> out;

        // iterate over dir entries, cast each entry to a char pointer
        for (auto entry: entries) {
            auto const ptr = reinterpret_cast<char*>(&entry);
            // append to the char vector
            out.insert(out.end(), ptr, ptr + sizeof entry);
        }

        return out;
    }


    // virtual destructor required, so that deleting child class instances invokes child destructor
    virtual ~VFS() = default;
};