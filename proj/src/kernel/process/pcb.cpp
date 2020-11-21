//
// Created by Stanislav Král on 18.11.2020.
//

#include <vector>
#include "pcb.h"

Process* Process_Control_Block::Add_Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out,
                                        char *program) {
    std::lock_guard<std::mutex> guard(mutex);
    table[handle] = std::make_unique<Process>(handle, std_in, std_out, program);
    printf("added %d", handle);
    return table[handle].get();
}

void Process_Control_Block::Remove_Process(kiv_os::THandle handle) {
    std::lock_guard<std::mutex> guard(mutex);
    table.erase(handle);
}

Process *Process_Control_Block::operator[](const kiv_os::THandle handle) {
    std::lock_guard<std::mutex> guard(mutex);
    return this->table[handle].get();
}

std::vector<Process *> Process_Control_Block::Get_Processes() {
    std::lock_guard<std::mutex> guard(mutex);
    std::vector<Process *> processes;

    printf("size %d\n", this->table.size());
    for (const auto &entry: this->table) {
        processes.push_back(entry.second.get());
        if (entry.second.get() == nullptr) {
            printf("never should have come here %d \n", entry.first);
        }
    }

    return processes;
}

