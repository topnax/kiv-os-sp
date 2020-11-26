//
// Created by Stanislav Král on 18.11.2020.
//

#pragma once

#include <map>
#include <mutex>
#include "../../api/api.h"
#include "process_entry.h"
#include "memory"

class Process_Control_Block {

public:
    /**
     * Creates and adds a process to the PCB based on given parameters
     *
     * @return Pointer to Process class instance
     */
    Process *Add_Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out, char *program);

    /**
     * Removes a process from the PCB based on the given handle
     * @param handle handle of the process to be removed
     */
    void Remove_Process(kiv_os::THandle handle);

    /**
     * Indexes to the underlying table, where key is the handle of the process and value is an instance of Process class
     *
     * @return pointer to the Process instance indexed by the given handle
     */
    Process *operator[](kiv_os::THandle);

    /**
     * Returns a vector of pointers to all Processes in the PCB
     * @return a vector of pointers to all Processes in the PCB
     */
    std::vector<Process *> Get_Processes();

    std::recursive_mutex *Get_Mutex();

    void procfs();

private:
    std::map<kiv_os::THandle, std::unique_ptr<Process>> table;
    std::recursive_mutex mutex;
};
