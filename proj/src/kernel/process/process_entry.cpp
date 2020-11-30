//
// Created by Stanislav Král on 18.11.2020.
//

#include "process_entry.h"

#include <utility>


Process::Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out, char *program_name, std::filesystem::path wd) :
        handle(handle), std_in(std_in), std_out(std_out), program_name(program_name), working_directory(std::move(wd)) {}
