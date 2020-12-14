#include <mutex>
#include <stack>
#include <random>
#include "files.h"
#include "tty_file.h"
#include "keyboard_file.h"
#include "proc_fs.h"
#include "fat_fs.h"
#include "dir.h"

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

//   Add_Filesystem(R"(C:\)", new Fat_Fs(0, kiv_hal::TDrive_Parameters{}));

  // TODO remove

    // find a fat drive
    kiv_hal::TRegisters regs{};
    for (regs.rdx.l = 0;; regs.rdx.l++) {
        kiv_hal::TDrive_Parameters params{};
        regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);
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
    Files::Filesystems[name] = std::unique_ptr<VFS>(vfs);
}

VFS *Get_Filesystem(const std::string& file_name) {
    auto resolved = Files::Filesystems.find(file_name);
    if (resolved != Files::Filesystems.end()) {
        return resolved->second.get();
    }

    return nullptr;
}

void Open_File(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);
    auto flags = static_cast<kiv_os::NOpen_File>(registers.rcx.l);
    auto attributes = static_cast<uint8_t>(registers.rdi.i);
    kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
    auto handle = Open_File(file_name, flags, attributes, error);

    if (error == kiv_os::NOS_Error::Success) {
        registers.rax.x = handle;
    } else {
        registers.flags.carry = 1;
        registers.rax.x = static_cast<decltype(registers.rax.x)>(error);
    }
}

kiv_os::THandle Open_File(const char *input_file_name, kiv_os::NOpen_File flags, uint8_t attributes, kiv_os::NOS_Error &error) {
    std::lock_guard<std::mutex> guard(Files::Open_Guard);
    Generic_File *file = nullptr;

    if (strcmp(input_file_name, "\\sys\\tty") == 0) {
        file = new Tty_File();
    } else if (strcmp(input_file_name, "\\sys\\kb") == 0) {
        file = new Keyboard_File();
    } else {
        std::filesystem::path resolved_path_relative_to_fs;
        std::filesystem::path absolute_path;

        std::filesystem::path input_path = input_file_name;
        std::string file_name = input_path.filename().string();

        // check whether the file has to exist in order to be opened
        if (flags != kiv_os::NOpen_File::fmOpen_Always) {
            // file does not have to exist
            if ((attributes & static_cast<decltype(attributes)>(kiv_os::NFile_Attributes::Directory) && File_Exists(input_path, resolved_path_relative_to_fs, absolute_path) != nullptr)) {
                // if a directory should be created a file at the given path mustn't exist
                error = kiv_os::NOS_Error::Invalid_Argument;
                return kiv_os::Invalid_Handle;
            }
            // when the file does not have to check whether the parent path does exist
            input_path = input_path.parent_path();
        }

        auto fs = File_Exists(input_path, resolved_path_relative_to_fs, absolute_path);
        if (fs != nullptr) {

            if (flags != kiv_os::NOpen_File::fmOpen_Always) {
                // add back the filename
                resolved_path_relative_to_fs /= file_name;
            }
            File f{};

            auto length = resolved_path_relative_to_fs.string().length() + 1;
            char *name = new char[length];
            strcpy_s(name, length, resolved_path_relative_to_fs.string().c_str());
            // open a file with the found filesystem

            auto result = fs->open(name, flags, attributes, f);
            if (result == kiv_os::NOS_Error::Success) {
                // file opened successfully
                file = new Filesystem_File(fs, f);
                f.name = name;
            } else {
                error = result;
                delete[] name;
            }
        } else {
            error = kiv_os::NOS_Error::File_Not_Found;
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
    } else {
        regs.flags.carry = 1;
        regs.rax.r = static_cast<decltype(regs.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
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
        auto res = file->write(buff, bytes, written);
        regs.rax.r = written;
        if (res != kiv_os::NOS_Error::Success) {
            // some error has happened
            regs.flags.carry = 1;
            regs.rax.r = static_cast<decltype(regs.rax.r)>(res);
        }
    } else {
        regs.rax.r = static_cast<decltype(regs.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
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
        auto res = file->read(buff_size, buff, read);
        regs.rax.r = read;

        if (res != kiv_os::NOS_Error::Success) {
            // some error has happened
            regs.rax.r = static_cast<decltype(regs.rax.r)>(res);
            regs.flags.carry = 1;
        }
    } else {
        regs.rax.r = static_cast<decltype(regs.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
        regs.flags.carry = 1;
    }
}

VFS *File_Exists(std::filesystem::path path, std::filesystem::path &path_relative_to_fs, std::filesystem::path &absolute_path) {

    // when path is relative, prepend the current working directory of the process
    if (path.is_relative()) {
        // std::filesystem::path does not support prepending
        std::filesystem::path wd_path;
        if (!Get_Working_Dir(wd_path)) {
            return nullptr;
        }
        wd_path.append(path.string());
        path = wd_path;
    }

    // create a variable `current_path`, which will hold the current path that will be build while iterating the components
    // in the given path

    // start with the root path of the given path
    auto current_path = path.root_path();

    // keep a stack of file systems, while we traverse the directory tree (we can find fs mount points)
    std::stack<VFS *> file_systems;

    // keep a stack of file systems, while we traverse the directory tree (we can find fs mount points)
    std::stack<std::filesystem::path> paths_relative_to_fs;

    // keep a stack of found cluster ids
    std::stack<int32_t> cluster_stack;

    // keep track of the current path relative
    std::filesystem::path current_fs_path;

    // find the fs for the current path
    VFS *current_fs = Get_Filesystem(current_path.string());

    file_systems.push(current_fs);

    if (current_fs != nullptr) {
        cluster_stack.push(current_fs->get_root_fd());
        // the first file lookup should start in the root of the fs
        bool first = true;

        // the current found fd
        int32_t current_fd = 0;

        // number of path components in the current fs - used determinate when to pop the `current_fs` stack
        int components_in_current_fs = 1;
        for (const auto &component : path.relative_path()) {
            if (component == "..") {
                // move one level up
                if (current_path.has_root_directory()) {
                    // when the current path has some file name, then it has atleast one component in the path
                    // that means we can move one level up

                    // remove the last filename
                    current_path = current_path.parent_path();

                    // decrement the number of components in the current fs
                    components_in_current_fs--;

                    cluster_stack.pop();

                    if (components_in_current_fs == 0) {
                        // no more components in the current fs, load the previous FS
                        file_systems.pop();
                        current_fs = file_systems.top();
                        components_in_current_fs = 1;

                        current_fs_path = paths_relative_to_fs.top();

                        // load the previous FS relative path too
                        paths_relative_to_fs.pop();

                        // current_fs->print_name();
                    } else {
                        current_fs_path = current_fs_path.parent_path();
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
                    // push the current path relative to fs into the stack
                    paths_relative_to_fs.push(current_fs_path);

                    // we found a FS mount point
                    file_systems.push(new_fs);
                    current_fs = new_fs;

                    cluster_stack.push(new_fs->get_root_fd());
                    // set the
                    current_fs_path = "\\";
                    components_in_current_fs = 1;

                } else {
                    current_fs_path /= component.string();
                    auto fd = cluster_stack.top();
                    // check whether current path relative to FS exists in the current fs
                    if (!current_fs->file_exists(fd, component.string().c_str(), first, current_fd)) {
                        return nullptr;
                    }
                    cluster_stack.push(current_fd);
                    components_in_current_fs++;
                }

                if (component.string() == ".") {
                    components_in_current_fs--;
                    current_path.remove_filename();
                }
            }
            first = false;
        }
        absolute_path = current_path;
        path_relative_to_fs = current_fs_path;

        return current_fs;
    }

    return nullptr;
}

void Seek(kiv_hal::TRegisters &registers) {
    kiv_os::THandle handle = registers.rdx.x;
    auto position = static_cast<size_t>(registers.rdi.r);
    auto pos_type = static_cast<kiv_os::NFile_Seek>(registers.rcx.l);
    auto op = static_cast<kiv_os::NFile_Seek>(registers.rcx.h);

    // check whether file exists
    if (Files::Ft->Exists(handle)) {
        size_t pos_from_start;
        auto file_object = (*Files::Ft)[handle];
        // try to perform seek
        auto result = file_object->seek(position, pos_type, op, pos_from_start);
        if (result == kiv_os::NOS_Error::Success) {
            registers.rax.r = pos_from_start;
            return;
        } else {
            registers.rax.r = static_cast<decltype(registers.rax.r)>(result);
        }
    } else {
        registers.rax.r = static_cast<decltype(registers.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
    }
    // file not found or seek has failed, set error
    registers.flags.carry = 1;
}

void Delete_File(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);
    std::filesystem::path resolved_path_relative_to_fs;
    std::filesystem::path absolute_path;

    auto fs = File_Exists(file_name, resolved_path_relative_to_fs, absolute_path);
    if (fs != nullptr) {
        // convert the resolved path into a char*
        auto length = resolved_path_relative_to_fs.string().length() + 1;
        char *name = new char[length];
        strcpy_s(name, length, resolved_path_relative_to_fs.string().c_str());

        // unlink a file with the found filesystem
        auto result = fs->rmdir(name);
        if (result == kiv_os::NOS_Error::Success) {
            // unlink successful, do not set error
            return;
        } else {
            registers.rax.r = static_cast<decltype(registers.rax.r)>(result);
        }
    } else {
        // file does not exist
        registers.rax.r = static_cast<decltype(registers.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
    }
    registers.flags.carry = 1;
}

void Set_File_Attributes(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);
    auto attributes = static_cast<uint8_t>(registers.rdi.i);

    std::filesystem::path resolved_path_relative_to_fs;
    std::filesystem::path absolute_path;

    auto fs = File_Exists(file_name, resolved_path_relative_to_fs, absolute_path);
    if (fs != nullptr) {
        // convert the resolved path into a char*
        auto length = resolved_path_relative_to_fs.string().length() + 1;
        char *name = new char[length];
        strcpy_s(name, length, resolved_path_relative_to_fs.string().c_str());

        // set file attributes with the found filesystem
        auto result = fs->set_attributes(file_name, attributes);
        if (result == kiv_os::NOS_Error::Success) {
            // attributes set successfully, do not set error
            return;
        } else {
            registers.rax.r = static_cast<decltype(registers.rax.r)>(result);
        }
    } else {
        registers.rax.r = static_cast<decltype(registers.rax.r)>(kiv_os::NOS_Error::File_Not_Found);
    }

    registers.flags.carry = 1;
}

void Get_File_Attributes(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);

    std::filesystem::path resolved_path_relative_to_fs;
    std::filesystem::path absolute_path;

    auto fs = File_Exists(file_name, resolved_path_relative_to_fs, absolute_path);
    if (fs != nullptr) {
        // convert the resolved path into a char*
        auto length = resolved_path_relative_to_fs.string().length() + 1;
        char *name = new char[length];
        strcpy_s(name, length, resolved_path_relative_to_fs.string().c_str());

        uint8_t attributes;

        auto result = fs->get_attributes(name, attributes);
        if (result == kiv_os::NOS_Error::Success) {
            registers.rdi.i = attributes;
            return;
        } else {
            registers.rax.x = static_cast<decltype(registers.rax.x)>(result);
        }
    } else {
        registers.rax.x = static_cast<decltype(registers.rax.x)>(kiv_os::NOS_Error::File_Not_Found);
    }

    registers.flags.carry = 1;
}
