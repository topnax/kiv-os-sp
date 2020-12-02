//
// Created by Stanislav Král on 06.11.2020.
//

#include "fat_fs.h"
#include "fat_fs_utils.h"
#include <iostream>

void Fat_Fs::init() {

    printf("################\n");
    printf("INIT FAT12 disk %d\n", disk_number);
    printf("param anos %d\n", disk_parameters.absolute_number_of_sectors);
    printf("param spt %d\n", disk_parameters.sectors_per_track);
    printf("param bps %d\n", disk_parameters.bytes_per_sector);
    printf("param c %d\n", disk_parameters.cylinders);
    printf(" param h %d\n", disk_parameters.heads);
    // init some FAT stuff :D
}

Fat_Fs::Fat_Fs(uint8_t disk_number, kiv_hal::TDrive_Parameters disk_parameters): disk_number(disk_number), disk_parameters(disk_parameters) {}

kiv_os::NOS_Error Fat_Fs::read(File file, size_t size, size_t offset, std::vector<char> &out) {
    int clust;

    Fat_Fs::file_exists(226, "\\FDISKPT.INI", false, false, clust); //JUST FYI - example usage - 226 is cluster of fdsetup\bin
    std::cout << "got cluster: " << clust;
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) {
    //rozdeleni na jednotlive polozky v ceste - START
    std::vector<std::string> folders_in_path; //vector obsahuje veskere slozky v absolutni ceste

    std::vector<char> one_item; //jedna slozka / polozka v absolutni ceste
    char character = 'A'; //jeden znak v ceste

    int counter = 0;

    while (true) { //dokud nedojdeme na konec stringu
        character = name[counter];

        if (character == '\\') { //konec nazvu jedne polozky v ceste
            std::string one_item_str(one_item.begin(), one_item.end()); //prevod vectoru na string pro nasledne ulozeni do pole

            if (one_item_str.size() != 0) {
                folders_in_path.push_back(one_item_str);
            }

            one_item.clear(); //vyresetovani obsahu bufferu pro jednu polozku
        }
        else if (character == '\0') { //konec stringu s cestou
            std::string one_item_str(one_item.begin(), one_item.end());

            folders_in_path.push_back(one_item_str);

            break; //konec stringu cesty, koncime
        }
        else { //pokracovani nazvu jednoho itemu, vlozit do bufferu
            one_item.push_back(character);
        }

        counter++;
    }
    //rozdeleni na jednotlive polozky v ceste - KONEC

    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::open(const char *name, uint8_t flags, uint8_t attributes, File &file) {
    // TODO implement this
    file = File {};
    file.name = const_cast<char*>(name);
    file.position = 0; //aktualni pozice, zaciname na 0

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat_Fs::mkdir(const char *name) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::rmdir(const char *name) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::write(File file, std::vector<char> buffer, size_t size, size_t offset, size_t &written) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::unlink(const char *name) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::close(File file) {
    return kiv_os::NOS_Error::IO_Error;
}

bool Fat_Fs::file_exists(int32_t current_fd, const char* name, bool start_from_root, bool is_folder, int32_t& found_fd)
{
    int start_cluster = -1;

    if (start_from_root) { //jdem z rootu, zacina na 19 clusteru
        start_cluster = 19;
    }
    else { //jdem z predanyho clusteru
        start_cluster = current_fd; 
    }

    //ziskani obsahu FAT tabulky pro vyhledavani sektoru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);

    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste

    directory_item dir_item = retrieve_item_clust(start_cluster, fat_table1_dec, is_folder, folders_in_path);
    found_fd = dir_item.first_cluster;

    if (dir_item.first_cluster == -1) { //polozka nenalezena
        return false;
    }
    else {
        return true;
    }
}
