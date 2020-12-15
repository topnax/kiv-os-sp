//
// Created by Stanislav Král on 02.12.2020.
//
#include "dir.h"
#include "files.h"
#include "process.h"

std::mutex Dir_Mutex;

bool Set_Working_Dir(kiv_hal::TRegisters &regs) {
    std::lock_guard<decltype(Dir_Mutex)> guard(Dir_Mutex);
    auto path = reinterpret_cast<char *>(regs.rdx.r);

    // check whether path has been correctly set
    if (path != nullptr) {

        auto process = resolve_current_thread_handle_to_process();
        if (process != nullptr) {
            // we have found a process which should have the working directory changed
            std::filesystem::path f_path = path;
            std::filesystem::path resolved_path_relative_to_fs;
            std::filesystem::path absolute_path;
            // check whether the given path exists
            if (File_Exists(f_path, resolved_path_relative_to_fs, absolute_path)) {
                process->working_directory = absolute_path;
                return true;
            }
        }
    }
    return false;
}

bool Get_Working_Dir(std::filesystem::path &out) {
    auto process = resolve_current_thread_handle_to_process();

    if  (process != nullptr) {
        // we found the parent process of this thread, fetch the wd
        out = process->working_directory;
        return true;
    }

    return false;
}

bool Get_Working_Dir(char *out_buffer, size_t buffer_size, size_t &written) {
    auto process = resolve_current_thread_handle_to_process();

    if  (process != nullptr) {
        // we found the parent process of this thread, copy the working directory into the provided buffer
        strcpy_s(out_buffer, buffer_size, process->working_directory.string().c_str());
        written = min(buffer_size, process->working_directory.string().length());
        return true;
    }

    return false;
}

bool Get_Working_Dir(kiv_hal::TRegisters &regs) {
    std::lock_guard<decltype(Dir_Mutex)> guard(Dir_Mutex);
    char *out_buffer = reinterpret_cast<char *>(regs.rdx.r);
    size_t buffer_size = static_cast<size_t>(regs.rcx.r);
    size_t written;

    if (Get_Working_Dir(out_buffer, buffer_size, written)) {
        // finding working dir of the current process succeeded, it now has been written to the buffer provided in the registers
        regs.rax.r = written;
        if (written > 0) {
            return true;
        }
    }

    return false;
}
