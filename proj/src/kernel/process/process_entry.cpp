//
// Created by Stanislav Král on 18.11.2020.
//

#include "process_entry.h"

#include <utility>


Process::Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out, char *program_name, std::filesystem::path wd) :
        handle(handle), std_in(std_in), std_out(std_out), program_name(program_name), working_directory(std::move(wd)) {}

PCB_Entry Process::Get_Pcb_Entry() {
    auto entry = PCB_Entry {
        this->handle,
        this->std_in,
        this->std_out,
    };

    strcpy_s(entry.program_name, 42, this->program_name);
    strcpy_s(entry.working_directory, 256, this->working_directory.string().c_str());

    return entry;
}
