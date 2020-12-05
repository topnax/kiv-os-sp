//
// Created by Stanislav Král on 06.11.2020.
//

#include <cstdio>
#include "proc_fs.h"
#include "process.h"
#include <algorithm>
#include <string>
#include <mutex>

constexpr size_t TASK_LIST_PCB_ENTRY_OUT_BUFFER_SIZE = 65;
constexpr int32_t PROCFS_ROOT = 0;

constexpr uint8_t ROOT_DIRECTORY_ATTRIBUTES = static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File) |
                                              static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only) |
                                              static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory);

constexpr uint8_t PCB_ENTRY_ATTRIBUTES = static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File) |
                                         static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only);


std::vector<char> Proc_Fs::generate_readproc_vector(Process p) {
    std::vector<char> out;

    // create a PCB entry from a process and insert it into a vector
    auto pcb_entry = p.Get_Pcb_Entry();
    auto const ptr = reinterpret_cast<char *>(&pcb_entry);
    // append to the char vector
    out.insert(out.end(), ptr, ptr + sizeof pcb_entry);

    return out;
}

kiv_os::NOS_Error Proc_Fs::read(File file, size_t size, size_t offset, std::vector<char> &out) {

    // prepare a vector of characters where the generated content will be stored
    std::vector<char> generated;

    if (strcmp(file.name, "\\") == 0 || strcmp(file.name, "\\.") == 0) {
        // reading procfs directory
        std::vector<kiv_os::TDir_Entry> entries;
        auto readdir_result = readdir("", entries);

        if (readdir_result == kiv_os::NOS_Error::Success) {
            generated = Proc_Fs::generate_dir_vector(entries);
        } else {
            return readdir_result;
        }
    } else {
        // reading a process info
        auto p = Get_Pcb()->operator[](file.handle);
        if (p != nullptr) {
            generated = generate_readproc_vector(*p);
        } else {
            return kiv_os::NOS_Error::File_Not_Found;
        }
    }

    // cannot read more that we have generated
    auto out_size = min(size, generated.size());

    // cannot read past past EOF
    auto read_end = min(offset + out_size, generated.size());

    out = std::vector<char>(generated.begin() + offset, generated.begin() + read_end);

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Proc_Fs::readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) {
    std::lock_guard<std::recursive_mutex> guard(*Get_Pcb()->Get_Mutex());
    auto pcb = Get_Pcb();
    for (Process *process : pcb->Get_Processes()) {
        auto entry = kiv_os::TDir_Entry{
                PCB_ENTRY_ATTRIBUTES
        };
        sprintf_s(entry.file_name, 12, "%d", process->handle);

        entries.push_back(entry);
    }

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Proc_Fs::open(const char *name, kiv_os::NOpen_File flags, uint8_t attributes, File &file) {
    // procfs ignores NOpen_File flags, because it does not allow creation of new files - the file always has to exist

    std::lock_guard<std::recursive_mutex> guard(*Get_Pcb()->Get_Mutex());
    auto pcb = Get_Pcb();

    // check whether opening the root folder - that should print the simplified PCB table
    if (strcmp(name, "\\") == 0 || strcmp(name, "\\.") == 0) {
        file = File{
                0,
                ROOT_DIRECTORY_ATTRIBUTES,
                sizeof(kiv_os::TDir_Entry) * pcb->Get_Processes().size(),
                0,
                const_cast<char *>(name),
        };
        // check attributes
        if ((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) == 0)
            return kiv_os::NOS_Error::Permission_Denied;
        return kiv_os::NOS_Error::Success;
    }

    // parse PID from the path
    int base = 10;
    char *str = const_cast<char *>(name + strlen("\\"));
    char *end;
    long int num;
    num = strtol(str, &end, base);
    // check whether there were no errors when parsing the file name to an integer
    if (!*end) {
        auto process = (*pcb)[static_cast<kiv_os::THandle>(num)];
        if (process != nullptr) {
            // we found the process that the reading of was requested
            file = File{
                    process->handle,
                    PCB_ENTRY_ATTRIBUTES,
                    sizeof(PCB_Entry),
                    0,
                    const_cast<char *>(name)
            };

            // read file must not be a directory
            if ((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0)
                return kiv_os::NOS_Error::Permission_Denied;
            return kiv_os::NOS_Error::Success;
        }
    }

    return kiv_os::NOS_Error::File_Not_Found;
}

kiv_os::NOS_Error Proc_Fs::mkdir(const char *name) {
    return kiv_os::NOS_Error::Permission_Denied;
}

kiv_os::NOS_Error Proc_Fs::rmdir(const char *name) {
    return kiv_os::NOS_Error::Permission_Denied;
}

kiv_os::NOS_Error Proc_Fs::write(File f, std::vector<char> buffer, size_t size, size_t offset, size_t &written) {
    return kiv_os::NOS_Error::Permission_Denied;
}

kiv_os::NOS_Error Proc_Fs::unlink(const char *name) {
    return kiv_os::NOS_Error::Permission_Denied;
}

kiv_os::NOS_Error Proc_Fs::close(File file) {
    // TODO what should close do?
    return kiv_os::NOS_Error::Success;
}

bool Proc_Fs::file_exists(int32_t current_fd, const char *name, bool start_from_root, int32_t &found_fd) {
    if (start_from_root) {
        current_fd = PROCFS_ROOT;
    }
    if (current_fd == PROCFS_ROOT) {
        if (strcmp(name, "tasklist") == 0) {
            found_fd = 0;
            return true;
        }

        if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
            return true;
        }

        // parse PID from the path
        int base = 10;
        char *end;
        long int num;
        num = strtol(name, &end, base);
        // check whether there were no errors when parsing the file name to an integer
        if (!*end) {
            auto process = (*Get_Pcb())[static_cast<kiv_os::THandle>(num)];
            if (process != nullptr) {
                found_fd = process->handle;
                return true;
            }
        }
    }
    return false;
}

kiv_os::NOS_Error Proc_Fs::get_attributes(const char *name, uint8_t &out_attributes) {
    if (strcmp(name, "\\") == 0 || strcmp(name, "\\.") == 0) {
        // root folder attributes
        out_attributes = ROOT_DIRECTORY_ATTRIBUTES;

        return kiv_os::NOS_Error::Success;
    }

    auto pcb = Get_Pcb();

    // parse PID from the path
    int base = 10;
    char *str = const_cast<char *>(name + strlen("\\"));
    char *end;
    long int num;
    num = strtol(str, &end, base);
    // check whether there were no errors when parsing the file name to an integer
    if (!*end) {
        auto process = (*pcb)[static_cast<kiv_os::THandle>(num)];
        if (process != nullptr) {
            out_attributes = PCB_ENTRY_ATTRIBUTES;
            return kiv_os::NOS_Error::Success;
        }
    }

    return kiv_os::NOS_Error::File_Not_Found;
}

kiv_os::NOS_Error Proc_Fs::set_attributes(const char *name, uint8_t attributes) {
    return kiv_os::NOS_Error::Permission_Denied;
}
