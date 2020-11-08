//
// Created by Stanislav Král on 06.11.2020.
//

#include "Pipe_File.h"

// Pipe_In_File method implementations
Pipe_In_File::Pipe_In_File(std::shared_ptr<Pipe> p) : pipe(std::move(p)) {}

size_t Pipe_In_File::write(char *buffer, size_t size) {
    // convert char* to vector<char>
    return pipe->Write(std::vector<char>(buffer, buffer + size));
}

void Pipe_In_File::close() {
    this->pipe->Close_In();
}


// Pipe_Out_File method implementations
Pipe_Out_File::Pipe_Out_File(std::shared_ptr<Pipe> p) : pipe(std::move(p)) {}

size_t Pipe_Out_File::read(size_t size, char *out_buffer) {
    // write contents of vector<char> to the given buffer
    auto data = pipe->Read(size / sizeof(char));
    for (int i = 0; i < data.size(); i++) {
        out_buffer[i] = data.at(i);
    }
    return data.size() * sizeof(char);
}

void Pipe_Out_File::close() {
    this->pipe->Close_Out();
}
