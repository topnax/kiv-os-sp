#include <mutex>
#include <stack>
#include <random>
#include "files.h"
#include "vga_file.h"
#include "keyboard_file.h"
#include "proc_fs.h"
#include "fat_fs.h"

namespace Files {
    std::mutex Open_Guard;
    std::recursive_mutex Access_Guard;
    File_Table *Ft = new File_Table();
    kiv_os::THandle Last_File = 0;
    std::map<std::string, std::unique_ptr<VFS>> Filesystems;
}

// File_Table class methods definitions

bool File_Table::Exists(kiv_os::THandle handle) {
    std::lock_guard<std::recursive_mutex> guard(Files::Access_Guard);
    return this->files.find(handle) != this->files.end();
}

kiv_os::THandle File_Table::Add_File(Generic_File *file) {
    std::lock_guard<std::recursive_mutex> guard(Files::Access_Guard);
    do {
        Files::Last_File++;
    } while (Exists(Files::Last_File));
    // find a handle that is unique

    // use unique_ptr so the object is deleted once removed from the map (map holds the ownership of the file)
    files[Files::Last_File] = std::unique_ptr<Generic_File>(file);

    // return the handle that was assigned to this file
    return Files::Last_File;
}

Generic_File *File_Table::operator[](const kiv_os::THandle handle) {
    std::lock_guard<std::recursive_mutex> guard(Files::Access_Guard);
    auto resolved = this->files.find(handle);
    return resolved != files.end() ? resolved->second.get() : nullptr;
}

void File_Table::Remove(const kiv_os::THandle handle) {
    std::lock_guard<std::recursive_mutex> guard(Files::Access_Guard);
    this->files.erase(handle);
}

// function definition

Generic_File *Resolve_THandle_To_File(kiv_os::THandle handle) {
    if (Files::Ft->Exists(handle)) {
        return (*Files::Ft)[handle];
    } else {
        return nullptr;
    }
}

void Init_Filesystems() {
    Add_Filesystem("C:\\procfs", new Proc_Fs());

   // Add_Filesystem(R"(C:\)", new Fat_Fs(0, kiv_hal::TDrive_Parameters{}));

   // // TODO remove
   // return;


    // find a fat drive
    kiv_hal::TRegisters regs;
    for (regs.rdx.l = 0;; regs.rdx.l++) {
        kiv_hal::TDrive_Parameters params;
        regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);;
        regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
        if (!regs.flags.carry) {
            // found a disk
            auto disk_num = regs.rdx.l;
            auto disk_params = reinterpret_cast<kiv_hal::TDrive_Parameters *>(regs.rdi.r);

            auto fs = new Fat_Fs(disk_num, *disk_params);
            fs->init();
            Add_Filesystem(R"(C:\)", fs);
            break;
        }

        if (regs.rdx.l == 255) break;
    }
}

void Add_Filesystem(const std::string &name, VFS *vfs) {
    printf("added %s to fs table\n", name.c_str());
    Files::Filesystems[name] = std::unique_ptr<VFS>(vfs);
}

VFS *Get_Filesystem(std::string file_name) {
    printf("resolving %s\n", file_name.c_str());
    auto resolved = Files::Filesystems.find(file_name);
    if (resolved != Files::Filesystems.end()) {
        printf("found %s\n", file_name.c_str());
        return resolved->second.get();
    }

    return nullptr;
}

void Open_File(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);
    uint8_t flags = registers.rcx.l;
    auto attributes = static_cast<uint8_t>(registers.rdi.i);
    registers.rax.x = Open_File(file_name, flags, attributes);
}

kiv_os::THandle Open_File(const char *file_name, uint8_t flags, uint8_t attributes) {
    std::lock_guard<std::mutex> guard(Files::Open_Guard);
    Generic_File *file = nullptr;



/*    printf("start #######################\n");
    std::filesystem::path resolved;
    if (File_Exists(R"(C:\procfs\..\procfs\1)", resolved) != nullptr) {
        printf("found %s at %s\n", file_name, resolved.string().c_str());
    }
    printf("end #######################\n");*/
    // TODO this could be improved in the future
    if (strcmp(file_name, "/dev/vga") == 0) {
        file = new Vga_File();
    } else if (strcmp(file_name, "/dev/kb") == 0) {
        file = new Keyboard_File();
    } else {
        std::filesystem::path resolved;
        auto fs = File_Exists(file_name, resolved);
        if (fs != nullptr) {
            printf("into fs\n");
            File f{};

            auto length = resolved.string().length() + 1;
            char *name = new char[length];
            strcpy_s(name, length, resolved.string().c_str());

            // open a file with the found filesystem
            auto result = fs->open(name, flags, attributes, f);
            if (result == kiv_os::NOS_Error::Success) {

                file = new Filesystem_File(fs, f);
                f.name = name;
            } else {
                delete[] name;
            }
        }
    }
    if (file == nullptr) {
        return kiv_os::Invalid_Handle;
    }
    return Files::Ft->Add_File(file);
}

void Close_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle handle = regs.rdx.x;
    if (Files::Ft->Exists(handle)) {
        auto file_object = (*Files::Ft)[handle];
        // close the file
        file_object->close();

        // remove it from the file table
        (*Files::Ft).Remove(handle);
    }
}

kiv_os::THandle Add_File_To_Table(Generic_File *file) {
    return Files::Ft->Add_File(file);
}

void Write_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle handle = regs.rdx.x;
    char *buff = reinterpret_cast<char *>(regs.rdi.r);
    auto bytes = static_cast<size_t>(regs.rcx.r);

    Generic_File *file = Resolve_THandle_To_File(handle);
    if (file != nullptr) {
        size_t written;
        bool res = file->write(buff, bytes, written);
        regs.rax.r = written;
        if (!res) {
            // some error has happened
            regs.flags.carry = 1;
        }
    } else {
        regs.rax.r = 0;
        regs.flags.carry = 1;
    }
}

void Read_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle handle = regs.rdx.x;

    Generic_File *file = Resolve_THandle_To_File(handle);

    if (file != nullptr) {
        auto buff_size = static_cast<size_t>(regs.rcx.r);
        char *buff = reinterpret_cast<char *>(regs.rdi.r);
        size_t read;
        bool res = file->read(buff_size, buff, read);
        regs.rax.r = read;
        if (!res) {
            // some error has happened
            regs.flags.carry = 1;
        }
    } else {
        regs.rax.r = 0;
        regs.flags.carry = 1;
    }
}

VFS *File_Exists(std::filesystem::path path, std::filesystem::path &absolute_path) {

    // when path is relative, prepend the current working directory of the process
    if (path.is_relative()) {
        // std::filesystem::path does not support prepending
        std::string wd = "C:\\";
        std::filesystem::path temp_path = wd;
        temp_path.append(path.string());
        path = temp_path;
    }

    // create a variable `current_path`, which will hold the current path that will be build while iterating the components
    // in the given path

    // start with the root path of the given path
    auto current_path = path.root_path();

    // keep a stack of file systems, while we traverse the directory tree (we can find fs mount points)
    std::stack<VFS *> file_systems;

    // keep a stack of file systems, while we traverse the directory tree (we can find fs mount points)
    std::stack<std::filesystem::path> paths_relative_to_fs;

    // keep track of the current path relative
    std::filesystem::path current_fs_path;

    // find the fs for the current path
    VFS *current_fs = Get_Filesystem(current_path.string());
//    if (current_fs == nullptr) {
//        current_fs = Get_Filesystem(path.parent_path().string());
//    }

    file_systems.push(current_fs);


    if (current_fs != nullptr) {
        // the first file lookup should start in the root of the fs
        bool first = true;

        // the current found fd
        int32_t current_fd = 0;

        // number of path components in the current fs - used determinate when to pop the `current_fs` stack
        int components_in_current_fs = 1;
        for (const auto &component : path.relative_path()) {
            if (component == "..") {
                // move one level up
                if (current_path.has_filename()) {
                    // when the current path has some file name, then it has atleast one component in the path
                    // that means we can move one level up

                    // remove the last filename
                    current_path.remove_filename();

                    // decrement the number of components in the current fs
                    components_in_current_fs--;

                    if (components_in_current_fs == 0) {
                        // no more components in the current fs, load the previous FS
                        file_systems.pop();
                        current_fs = file_systems.top();

                        printf("trying to get the top\n");
                        current_fs_path = paths_relative_to_fs.top();

                        // load the previous FS relative path too
                        printf("about to pop relative path stacksize=%d\n", paths_relative_to_fs.size());
                        paths_relative_to_fs.pop();

                        printf("popped to fs: ");
                        current_fs->print_name();
                        printf("popped path is: %s\n", current_fs_path.string().c_str());
                    }
                } else {
                    // cannot pop out of the root folder
                    return nullptr;
                }
            } else {
                // append the next component
                current_path /= component.string();

                // check whether the current path is a FS mount point
                auto new_fs = Get_Filesystem(current_path.string());
                if (new_fs != nullptr) {
                    printf("pushing :)\n");
                    // push the current path relative to fs into the stack
                    paths_relative_to_fs.push(current_fs_path);

                    // we found a FS mount point
                    file_systems.push(new_fs);
                    current_fs = new_fs;

                    // set the
                    printf("stack size: %d\n", paths_relative_to_fs.size());
                    current_fs_path = "\\";
                    printf("stack size: %d\n", paths_relative_to_fs.size());
                    components_in_current_fs = 1;

                    // printf("changed to fs ");
                    // current_fs->print_name();
                } else {
                    current_fs_path /= component.string();
                    if (!current_fs->file_exists(current_fd, component.string().c_str(), first, current_fd)) {
                        return nullptr;
                    }
                    components_in_current_fs++;
                }
            }
            first = false;
        }
        absolute_path = current_fs_path;
        return current_fs;
    }

    return nullptr;
}
