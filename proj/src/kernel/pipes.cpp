#include <map>
#include <random>
#include "pipes.h"
#include "Pipe_File.h"
#include "files.h"

const size_t PIPE_BUFFER_SIZE_BYTES = 64000;

std::pair<kiv_os::THandle, kiv_os::THandle> Create_Pipe() {
    // create a shared pointer to an instance of a pipe,
    // pipe file objects hold the ownership of the pipe instance,
    // the pipe is deleted once pipe file objects are automatically deleted from the file table
    auto p = std::make_shared<Pipe>(PIPE_BUFFER_SIZE_BYTES / sizeof(char));

    // create a file object for pipe IN
    Generic_File *pipe_in_file = new Pipe_In_File(p);

    // create a file object for pipe OUT
    Generic_File *pipe_out_file = new Pipe_Out_File(p);

    // add files to the file table
    auto in_handle = Add_File_To_Table(pipe_in_file);
    auto out_handle = Add_File_To_Table(pipe_out_file);

    // return in/out handles as a pair
    return std::make_pair(in_handle, out_handle);
}

