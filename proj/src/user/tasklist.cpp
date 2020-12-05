//
// Created by Stanislav Král on 31.10.2020.
//

#include <vector>
#include <string>
#include "rtl.h"

// size of the buffer used to hold formatted PCB entry to be written to std_out
constexpr size_t TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE = 86;

///////////////////////////////////////
/// these should be defined in api.h.
/// for now both the following enum and structure are hardcoded and shared between userspace and kernel
enum class Process_Status {
    Ready = 0,
    Running = 1,
    Zombie = 2
};

struct PCB_Entry {
    kiv_os::THandle handle;
    kiv_os::THandle std_in;
    kiv_os::THandle std_out;
    Process_Status status;
    kiv_os::NOS_Error exit_code;
    char program_name[42];
    char working_directory[256];
};
/// to be added to api.h in the future KIV/OS SP iterations
////////////////////////////////////////

// reads procfs directory items and returns them in a vector container
std::vector<kiv_os::TDir_Entry> read_procfs_dir_items() {
    kiv_os::THandle handle;

    std::vector<kiv_os::TDir_Entry> dir_items;

    // directory file contains information about the directory itself, read the whole file
    auto attributes = static_cast<uint8_t>(static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));

    if (kiv_os_rtl::Open_File("C:\\procfs", kiv_os::NOpen_File::fmOpen_Always, attributes, handle)) {
        // get the size of the TDir_Entry structure
        const auto dir_entry_size = sizeof(kiv_os::TDir_Entry);
        size_t read;

        // prepare in which we will read
        char read_buffer[dir_entry_size];

        // read directory items till EOF
        while (kiv_os_rtl::Read_File(handle, read_buffer, dir_entry_size, read)) {
            if (read == dir_entry_size) {
                auto *entry = reinterpret_cast<kiv_os::TDir_Entry *>(read_buffer);
                dir_items.push_back(*entry);
            }
        }

        kiv_os_rtl::Close_Handle(handle);
    }
    return dir_items;
}

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters &regs) {
    const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

    // read procfs directory items
    auto procfs_dir_items = read_procfs_dir_items();

    if (!procfs_dir_items.empty()) {
        // PCB correctly read and contains atleast one item

        // prepare a buffer for the path to an entry in the PCB table
        char pcb_item_path[42];

        // a variable used to store the handle later used to read a PCB entry
        kiv_os::THandle pcb_item_handle;

        // size of a PCB entry structure
        const auto pcb_entry_size = sizeof(PCB_Entry);

        // buffer in which a PCB entry will be  read
        char pcb_entry_buffer[pcb_entry_size];

        // a variable used for storing the amount of bytes we have read from a file containing a PCB entry
        size_t pcb_read;

        // a variable used for storing the amount of bytes we have written to a std_out
        size_t written_to_out;

        // prepare process status labels
        std::string statuses[] = {
                "Ready", "Running", "Zombie"
        };

        // a buffer used to hold text to be written to std_out
        char pcb_entry_out_buffer[TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE];

        // format to be used when printing a PCB entry
        const char *pcb_entry_format = "|%-7d |%-10s |%-9s |%-7d |%-7d |%-10hu |%-20s|\n";

        // format to be used when printing the PCB header
        const char *pcb_header_format = "|%-7s |%-10s |%-9s |%-7s |%-7s |%-10s |%-20s|\n";

        // write and print the PCB table header
        sprintf_s(pcb_entry_out_buffer, TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE,
                  pcb_header_format,
                  "PID",
                  "PROGRAM",
                  "STATUS",
                  "STD IN",
                  "STD OUT",
                  "EXIT CODE",
                  "WORKING DIRECTORY");
        kiv_os_rtl::Write_File(std_out, pcb_entry_out_buffer, TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE - 1, written_to_out);

        // write the table header separator
        const char *separator = "====================================================================================\n";
        kiv_os_rtl::Write_File(std_out, separator, TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE - 1, written_to_out);

        for (auto dir_item: procfs_dir_items) {
            // generate the path to the currently iterated directory item representing an entry in the PCB table
            sprintf_s(pcb_item_path, 42, "C:\\procfs\\%s", dir_item.file_name);
            if (kiv_os_rtl::Open_File(pcb_item_path, kiv_os::NOpen_File::fmOpen_Always, 0, pcb_item_handle)) {
                // file containing a table entry opened successfully
                if (kiv_os_rtl::Read_File(pcb_item_handle, pcb_entry_buffer, pcb_entry_size, pcb_read) &&
                    pcb_read == pcb_entry_size) {
                    // if the read operation was successful and we have read one whole PCB entry, write it to std_out
                    auto *pcb_entry = reinterpret_cast<PCB_Entry *>(pcb_entry_buffer);
                    sprintf_s(pcb_entry_out_buffer, TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE,
                              pcb_entry_format,
                              pcb_entry->handle,
                              pcb_entry->program_name,
                              statuses[(int) pcb_entry->status].c_str(),
                              pcb_entry->std_in, pcb_entry->std_out,
                              pcb_entry->exit_code,
                              pcb_entry->working_directory);
                    kiv_os_rtl::Write_File(std_out, pcb_entry_out_buffer, TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE - 1, written_to_out);
                }

                // close the opened pcb item handle
                kiv_os_rtl::Close_Handle(pcb_item_handle);
            }
        }
    } else {
        size_t written;
        const char *message = "Failed to open PCB table or PCB table is empty\n";
        kiv_os_rtl::Write_File(std_out, message, strlen(message), written);
    }

    return 0;
}
