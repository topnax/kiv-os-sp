#include <mutex>
#include <random>
#include "files.h"
#include "Pipe_File.h"
#include "Vga_File.h"
#include "Keyboard_File.h"

namespace Files {
    std::mutex Open_Guard;
    std::mutex Add_Guard;
    File_Table *ft = new File_Table();
    kiv_os::THandle Last_File = 0;
}

// File_Table class methods definitions

bool File_Table::Exists(kiv_os::THandle handle) {
    return this->files.find(handle) != this->files.end();
}

kiv_os::THandle File_Table::Add_File(Generic_File *file) {
    std::lock_guard<std::mutex> guard(Files::Add_Guard);
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
    return this->files[handle].get();
}

void File_Table::Remove(const kiv_os::THandle handle) {
    this->files.erase(handle);
}

// function definition

Generic_File *Resolve_THandle_To_File(kiv_os::THandle handle) {
    if (Files::ft->Exists(handle)) {
        return (*Files::ft)[handle];
    } else {
        return nullptr;
    }
}

kiv_os::THandle Open_File(const char *file_name, uint8_t flags, uint8_t attributes) {
    std::lock_guard<std::mutex> guard(Files::Open_Guard);
    // TODO use flags&attributes
    Generic_File *file;

    // TODO this could be improved in the future
    if (strcmp(file_name, "/dev/vga") == 0) {
        file = new Vga_File();
    } else if (strcmp(file_name, "/dev/kb") == 0) {
        file = new Keyboard_File();
    } else {
        // TODO open a file from FS
        return kiv_os::Invalid_Handle;
    }
    return Files::ft->Add_File(file);
}

void Close_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle handle = regs.rdx.x;
    if (Files::ft->Exists(handle)) {
        auto file_object = (*Files::ft)[handle];
        // close the file
        file_object->close();

        // remove it from the file table
        (*Files::ft).Remove(handle);
    }
}

kiv_os::THandle Add_File_To_Table(Generic_File *file) {
    return Files::ft->Add_File(file);
}

void Write_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle handle = regs.rdx.x;
    char *buff = reinterpret_cast<char *>(regs.rdi.r);
    size_t bytes = regs.rcx.r;

    Generic_File *file = Resolve_THandle_To_File(handle);
    if (file != nullptr) {
        regs.rax.r = file->write(buff, bytes);
    } else {
        regs.rax.r = -1;
    }
}

void Read_File(kiv_hal::TRegisters &regs) {
    kiv_os::THandle  handle = regs.rdx.x;

    Generic_File *file = Resolve_THandle_To_File(handle);

    if (file != nullptr) {
        size_t buff_size = regs.rcx.r;
        char *buff = reinterpret_cast<char *>(regs.rdi.r);
        size_t read = file->read(buff_size, buff);
        regs.rax.r = read;
    } else {
        regs.rax.r = -1;
    }
}
