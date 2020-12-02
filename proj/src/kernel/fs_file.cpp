//
// Created by Stanislav Král on 25.11.2020.
//
#include "fs_file.h"
#include "algorithm"

Filesystem_File::Filesystem_File(VFS *vfs, File file) : vfs(vfs), file(file) {}

bool Filesystem_File::read(size_t to_read, char *out_buffer, size_t &read) {
    std::vector<char> out;

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

    return result == kiv_os::NOS_Error::Success && read > 0;
}

kiv_os::NOS_Error Filesystem_File::seek(size_t value, kiv_os::NFile_Seek seek_position, kiv_os::NFile_Seek op, size_t &res) {
    switch (op) {
        case kiv_os::NFile_Seek::Get_Position:
            res = file.position;
            break;
        case kiv_os::NFile_Seek::Set_Size:
            file.size = value;
            file.position = value;
            // TODO fs resize ???
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

    // TODO should this be always success?
    return kiv_os::NOS_Error::Success;
}


bool Filesystem_File::write(char *buffer, size_t buf_size, size_t &written) {
    // copy buffer into a vector
    std::vector<char> buf(buffer, buffer + buf_size);

    // write into the file system
    auto result = vfs->write(file, buf, buf_size, file.position, written);

    // move the position boy the amount of bytes we have written
    file.position += written;

    return result == kiv_os::NOS_Error::Success;
}

void Filesystem_File::close() {
    // TODO close?
}

Filesystem_File::~Filesystem_File() {
    delete this->file.name;
}
