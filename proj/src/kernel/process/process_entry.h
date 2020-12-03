//
// Created by Stanislav Král on 18.11.2020.
//

#pragma once


#include <map>
#include "../../api/api.h"
#include <filesystem>

enum class Process_Status {
    Ready = 0,
    Running = 1,
    Zombie = 2
};

struct PCB_Entry {
    kiv_os::THandle handle;
    kiv_os::THandle std_in;
    kiv_os::THandle std_out;
    char program_name[42];
    char working_directory[256];
};

class Process {

public:
    Process(kiv_os::THandle handle, kiv_os::THandle std_in, kiv_os::THandle std_out, char *program_name, std::filesystem::path working_directory);

    /**
     * Handle of this process
     */
    kiv_os::THandle handle;

    /**
     * Name of the program
     */
    char *program_name;

    /**
     * STD_IN handle of this process
     */
    kiv_os::THandle std_in;

    /**
     * STD_OUT handle of this process
     */
    kiv_os::THandle std_out;

    /**
     * Map of this process' signal handlers
     */
    std::map<kiv_os::NSignal_Id, kiv_os::TThread_Proc> signal_handlers;

    /**
     * Exit code of this process (implicitly set to NOS_Error::Success)
     */
    kiv_os::NOS_Error exit_code = kiv_os::NOS_Error::Success;

    /**
     * Status of the process
     */
     Process_Status status = Process_Status::Ready;

     /**
      * Path to current working directory
      */
     std::filesystem::path working_directory;

     PCB_Entry Get_Pcb_Entry();

};

