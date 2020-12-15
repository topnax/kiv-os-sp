//
// Created by Stanislav Král on 25.11.2020.
//
#include "fs_file.h"
#include "algorithm"

Filesystem_File::Filesystem_File(VFS *vfs, File file) : vfs(vfs), file(file) {}

kiv_os::NOS_Error Filesystem_File::read(size_t to_read, char *out_buffer, size_t &read) {
    std::vector<char> out;

    to_read = std::min(to_read, file.size - file.position);
    if (to_read <= 0) {
        // trying to read out of file's bounds
        return kiv_os::NOS_Error::IO_Error;
    }

    // read from fs
    auto result = vfs->read(file, to_read, file.position, out);

    // move the data to the given buffer
    for (size_t i = 0; i < out.size(); i++) {
        out_buffer[i] = out.at(i);
    }

    // set how many bytes we have read
    read = out.size();

    // move the position by the amount of bytes we have read
    file.position += read;

    if (read > 0 && result == kiv_os::NOS_Error::Success) {
        return kiv_os::NOS_Error::Success;
    }
    return result;
}

kiv_os::NOS_Error Filesystem_File::seek(size_t value, kiv_os::NFile_Seek seek_position, kiv_os::NFile_Seek op, size_t &res) {
    switch (op) {
        case kiv_os::NFile_Seek::Get_Position:
            res = file.position;
            break;
        case kiv_os::NFile_Seek::Set_Size:
            if (is_read_only()) {
                return kiv_os::NOS_Error::Permission_Denied;
            }
            file.size = value;
            file.position = value;
            break;

        default:
        case kiv_os::NFile_Seek::Set_Position:
            switch (seek_position) {

                default:
                case kiv_os::NFile_Seek::Beginning:
                    // position should be equal to the value passed
                    file.position = value;
                    break;

                case kiv_os::NFile_Seek::Current:
                    // the given value should be added to the position, but must not exceed the file size
                    file.position = std::max(file.size, file.position + value);
                    break;
                case kiv_os::NFile_Seek::End:
                    file.position = file.size - value;
                    break;
            }
            break;
    }

    return kiv_os::NOS_Error::Success;
}


kiv_os::NOS_Error Filesystem_File::write(char *buffer, size_t buf_size, size_t &written) {
    if (is_read_only() || is_directory()) {
        return kiv_os::NOS_Error::Permission_Denied;
    }
//    printf("about to write size=%d, offset=%d \"%s\"\n", buf_size, file.position,buffer);
    // copy buffer into a vector
    std::vector<char> buf(buffer, buffer + buf_size);

    // write into the file system
    auto result = vfs->write(file, buf, buf_size, file.position, written);
    // move the position boy the amount of bytes we have written
    file.position += written;
    file.size += written;

    return result;
}

void Filesystem_File::close() {
}

Filesystem_File::~Filesystem_File() {
    delete this->file.name;
}

bool Filesystem_File::has_attribute(kiv_os::NFile_Attributes attribute) const {
    return (static_cast<uint8_t>(this->file.attributes) & static_cast<uint8_t>(attribute)) != 0;
}

bool Filesystem_File::is_read_only() {
    return has_attribute(kiv_os::NFile_Attributes::Read_Only);
}

bool Filesystem_File::is_directory() {
    return has_attribute(kiv_os::NFile_Attributes::Directory);
}

bool Filesystem_File::is_hidden() {
    return has_attribute(kiv_os::NFile_Attributes::Hidden);
}

bool Filesystem_File::is_system_file() {
    return has_attribute(kiv_os::NFile_Attributes::System_File);
}

bool Filesystem_File::is_volume_id() {
    return has_attribute(kiv_os::NFile_Attributes::Volume_ID);
}

bool Filesystem_File::is_archive() {
    return has_attribute(kiv_os::NFile_Attributes::Archive);
}