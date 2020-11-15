#pragma once


#include <map>
#include <memory>

#include "generic_file.h"
#include "pipe_file.h"

// a class representing the file table
class File_Table {
public:
    // override the [] operator
    Generic_File *operator[](kiv_os::THandle);

    // ads a file to the table
    kiv_os::THandle Add_File(Generic_File *file);

    // returns true when the given handle is found in the table
    bool Exists(kiv_os::THandle handle);

    // removes the given handle from the table
    void Remove(kiv_os::THandle handle);

protected:
    std::map<kiv_os::THandle, std::unique_ptr<Generic_File>> files;
};


// function definitions

Generic_File* Resolve_THandle_To_File(kiv_os::THandle handle);

kiv_os::THandle Open_File(const char *file_name, uint8_t flags, uint8_t attributes);

kiv_os::THandle Add_File_To_Table(Generic_File *file);

void Close_File(kiv_hal::TRegisters &registers);

void Write_File(kiv_hal::TRegisters &registers);

void Read_File(kiv_hal::TRegisters &registers);