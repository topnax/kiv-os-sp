#include <mutex>
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
    Add_Filesystem("procfs", new Proc_Fs());

    // find a fat drive
    kiv_hal::TRegisters regs;
    for (regs.rdx.l = 0; ; regs.rdx.l++) {
        kiv_hal::TDrive_Parameters params;
        regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);;
        regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
        if (!regs.flags.carry) {
            // found a disk
            auto disk_num = regs.rdx.l;
            auto disk_params = reinterpret_cast<kiv_hal::TDrive_Parameters*>(regs.rdi.r);

            auto fs = new Fat_Fs(disk_num, *disk_params);
            fs->init();
            Add_Filesystem("fatfs", fs);

            //pokus o precteni \FDSETUP\BIN\ATTRIB.COM
            File file_test;
            file_test.size = 5044; //velikost souboru
            file_test.name = "\\FDSETUP\\BIN"; //cesta soubor
            file_test.position = 0; //aktualni pozice, zaciname na 0

            std::vector<char> out_buffer; //buffer pro prectene informace
            //fs->read(file_test, 512, 0, out_buffer); //instance File, velikost pro cteni, offset, out buffer

            File test_file;
            std::vector<char> test_out_buffer; //buffer pro prectene informace
            fs -> open("\\FDSETUP\\SETUP\\PACKAGES\\WELCOME.ZIP", 0, 0, test_file); //name, flags, attributes, file
            fs -> read(test_file, 512, 0, test_out_buffer); //file, size, offset, out buffer
   
            break;
        }

        if (regs.rdx.l == 255) break;
    }
}

void Add_Filesystem(const std::string& name, VFS *vfs) {
    Files::Filesystems[name] = std::unique_ptr<VFS>(vfs);
}

VFS* Get_Filesystem(const char *file_name) {
    auto resolved = Files::Filesystems.find(std::string(file_name));
    if (resolved != Files::Filesystems.end()) {
        return resolved->second.get();
    }

    return nullptr;
}

kiv_os::THandle Open_File(const char *file_name, uint8_t flags, uint8_t attributes) {
    std::lock_guard<std::mutex> guard(Files::Open_Guard);
    Generic_File *file = nullptr;

    // TODO this could be improved in the future
    if (strcmp(file_name, "/dev/vga") == 0) {
        file = new Vga_File();
    } else if (strcmp(file_name, "/dev/kb") == 0) {
        file = new Keyboard_File();
    } else {
        VFS *fs = nullptr;
        // TODO relative path
        // find filesystem for this path
        // files starting with /procfs should be used with procfs vfs
        if (strncmp(file_name, "/procfs", 6) == 0) {
            fs = Get_Filesystem("procfs");
        } else {
            fs = Get_Filesystem("fatfs");
        }

        if (fs != nullptr) {
            File f{};
            // open a file with the found filesystem
            auto result = fs->open(file_name, flags, attributes , f);
            if (result == kiv_os::NOS_Error::Success) {
                file = new Filesystem_File(fs, f);
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
    kiv_os::THandle  handle = regs.rdx.x;

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
