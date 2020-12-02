//
// Created by Stanislav Král on 02.12.2020.
//
#include "dir.h"
#include "files.h"
#include "process.h"

bool Set_Working_Dir(kiv_hal::TRegisters &regs) {
    auto path = reinterpret_cast<char *>(regs.rdx.r);

    // check whether path has been correctly set
    if (path != nullptr) {

        // resolve the handle of the current native thread
        kiv_os::THandle thread_handle;
        bool thread_handle_resolved = resolve_native_thread_id_to_thandle(std::this_thread::get_id(), thread_handle);

        if (thread_handle_resolved) {
            // native thread has been mapped to THandle

            Process *process = nullptr;
            auto pcb = Get_Pcb();

            // check whether the current native thread is a kiv_os process
            auto p = pcb->operator[](thread_handle);
            if (p != nullptr) {
                // it really is a process
                process = p;
            }  else {
                // it might be kiv_os thread, check whether the thread has a parent pid assigned
                auto tcb = Get_Tcb();
                auto resolved_thread_thandle = tcb->Parent_Processes.find(thread_handle);
                if (resolved_thread_thandle != tcb->Parent_Processes.end()) {
                    // current kiv_os thread has a pid assigned, find the process using the pid
                    process = pcb->operator[](resolved_thread_thandle->second);
                }
            }

            if (process != nullptr) {
                // we have found a process which should have the working directory changed
                std::filesystem::path f_path = path;
                std::filesystem::path resolved_path_relative_to_fs;
                std::filesystem::path absolute_path;
                // check whether the given path exists
                if (File_Exists(f_path, resolved_path_relative_to_fs, absolute_path)) {
                    process->working_directory = absolute_path;
                    printf("absolute path of %d set to: %s\n", process->handle, absolute_path.string().c_str());
                    return true;
                }
            }
        }
    }
    return false;
}
