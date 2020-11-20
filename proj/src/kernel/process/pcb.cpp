//
// Created by Stanislav Král on 18.11.2020.
//

#include <vector>
#include "pcb.h"

Process* Process_Control_Block::Add_Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out,
                                        char *program) {
    table[handle] = std::make_unique<Process>(handle, std_in, std_out, program);
    return table[handle].get();
}

void Process_Control_Block::Remove_Process(kiv_os::THandle handle) {
    table.erase(handle);
}

Process *Process_Control_Block::operator[](const kiv_os::THandle handle) {
    return this->table[handle].get();
}

std::vector<Process *> Process_Control_Block::Get_Processes() {
    std::vector<Process *> processes;

    for (const auto &entry: this->table) {
        processes.push_back(entry.second.get());
    }

    return processes;
}

