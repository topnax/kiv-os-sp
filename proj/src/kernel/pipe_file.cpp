//
// Created by Stanislav Král on 06.11.2020.
//

#include "pipe_file.h"

// Pipe_In_File method implementations
Pipe_In_File::Pipe_In_File(std::shared_ptr<Pipe> p) : pipe(std::move(p)) {}

bool Pipe_In_File::write(char *buffer, size_t size, size_t &written) {
    // convert char* to vector<char>
    written =  pipe->Write(std::vector<char>(buffer, buffer + size));
    return written > 0;
}

void Pipe_In_File::close() {
    this->pipe->Close_In();
}

kiv_os::NOS_Error Pipe_In_File::seek(size_t value, kiv_os::NFile_Seek position, kiv_os::NFile_Seek op, size_t &res) {
    return kiv_os::NOS_Error::IO_Error;
}


// Pipe_Out_File method implementations
Pipe_Out_File::Pipe_Out_File(std::shared_ptr<Pipe> p) : pipe(std::move(p)) {}

bool Pipe_Out_File::read(size_t size, char *out_buffer, size_t &read) {
    // write contents of vector<char> to the given buffer
    auto data = pipe->Read(size / sizeof(char));
    for (size_t i = 0; i < data.size(); i++) {
        out_buffer[i] = data.at(i);
    }
    read = data.size() * sizeof(char);
    return true;
}

void Pipe_Out_File::close() {
    this->pipe->Close_Out();
}
