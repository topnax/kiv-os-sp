//
// Created by Stanislav Král on 18.11.2020.
//

#include <vector>
#include "pcb.h"

const auto DEFAULT_WD = "C:\\";

Process *Process_Control_Block::Add_Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out,
                                            char *program) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    table[handle] = std::make_unique<Process>(handle, std_in, std_out, program, DEFAULT_WD);
    return table[handle].get();
}

void Process_Control_Block::Remove_Process(kiv_os::THandle handle) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    table.erase(handle);
}

Process *Process_Control_Block::operator[](const kiv_os::THandle handle) {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    auto resolved = this->table.find(handle);
    return resolved != this->table.end() ? resolved->second.get() : nullptr;
}

std::vector<Process *> Process_Control_Block::Get_Processes() {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    std::vector<Process *> processes;

    for (const auto &entry: this->table) {
        processes.push_back(entry.second.get());
    }

    return processes;
}

