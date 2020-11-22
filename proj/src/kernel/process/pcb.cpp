//
// Created by Stanislav Král on 18.11.2020.
//

#include <vector>
#include "pcb.h"

Process *Process_Control_Block::Add_Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out,
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


void Process_Control_Block::procfs() {
    std::lock_guard<std::mutex> guard(mutex);
    std::string statuses[] = {
            "Ready", "Running", "Zombie"
    };
// TODO remove for release, move to procfs
    printf("|%.*s|\n", 60,
           "========================================================================================================================");
    printf("|%-7s |%-10s |%-9s |%-7s |%-7s |%-10s|\n", "HANDLE", "PROGRAM", "STATUS", "STD_IN", "STD_OUT", "EXIT_CODE");
    printf("|%.*s|\n", 60,
           "========================================================================================================================");
        for (const auto &entry: this->table) {
            auto p = entry.second.get();
            if (p == nullptr) {
                printf("found nullptr %d\n", entry.first);
            }
//printf("|%-7d |%-10s |%-9s |%-7d |%-7d |%-10hu|\n", p->handle, p->program_name, statuses[(int) p->status].c_str(), p->std_in, p->std_out, p->exit_code);
        printf("|%-7d |%-10s |%-9s |%-7d |%-7d |%-10hu|\n", p->handle, p->program_name,
               statuses[(int) p->status].c_str(), p->std_in, p->std_out, p->exit_code);
    }
    printf("|%.*s|\n", 60,
           "========================================================================================================================");
}