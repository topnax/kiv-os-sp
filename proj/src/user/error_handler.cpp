//
// Created by Stanislav Král on 05.12.2020.
//

#include "error_handler.h"
#include "rtl.h"

void handle_error_message(kiv_os::NOS_Error error, kiv_os::THandle std_out) {
    switch (error) {
        case kiv_os::NOS_Error::File_Not_Found: {
            const char *error_message = "File not found.\n";
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }
        case kiv_os::NOS_Error::Directory_Not_Empty: {
            const char *error_message = "Directory not empty.\n";
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }
        case kiv_os::NOS_Error::IO_Error: {
            const char *error_message = "IO error has happened.\n";
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }
        case kiv_os::NOS_Error::Permission_Denied: {
            const char *error_message = "Permission denied.\n";
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }
        case kiv_os::NOS_Error::Not_Enough_Disk_Space: {
            const char *error_message = "Not enough disk space.\n";
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }

        default: {
            char error_message[42];
            sprintf_s(error_message, 42, "Unknown error %u.\n", error);
            size_t written;
            kiv_os_rtl::Write_File(std_out, error_message, strlen(error_message), written);
            break;
        }
    }
}

