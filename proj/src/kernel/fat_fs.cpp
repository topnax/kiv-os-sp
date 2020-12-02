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
    std::cout << "about to read from cluster" << file.handle;
    //Fat_Fs::file_exists(226, "\\FDISKPT.INI", false, false, clust); //JUST FYI - example usage - 226 is cluster of fdsetup\bin
    //std::cout << "got cluster: " << clust;
    
    //ziskani obsahu FAT tabulky pro vyhledavani sektoru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
    
    std::vector<int> file_clust_nums = retrieve_sectors_nums_fs(fat_table1_dec, file.handle); //ziskani seznamu clusteru, na kterych se soubor nachazi
    std::cout << "Retrieved clusts - START\n";
    for (int i = 0; i < file_clust_nums.size(); i++) {
        std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        std::cout << "Item is: " << file_clust_nums.at(i) << "\n";
    }
    std::cout << "Retrieved clusts - END\n";


    std::vector<unsigned char> file_one_clust; //obsah jednoho clusteru souboru
    std::vector<unsigned char> file_all_clust; //obsah veskerych clusteru souboru

    for (int i = 0; i < file_clust_nums.size(); i++) { //pruchod vsemi clustery, pres ktere je soubor natazen
        file_one_clust = read_data_from_fat_fs(file_clust_nums[i], 1); //ziskani bajtu souboru v ramci jednoho sektoru

        for (int j = 0; j < file_one_clust.size(); j++) { //vlozeni obsahu jednoho sektoru do celkoveho seznamu bajtu
            file_all_clust.push_back(file_one_clust.at(j));
        }
    }

    for (int i = 0; i < file_all_clust.size(); i++) {
        std::cout << file_all_clust.at(i);
    }

    //mame cely obsah souboru - vratit jen pozadovanou cast
    int current_byte = offset; //cteme od offset
    int to_read = -1;

    if (size == 0 || (file_all_clust.size() - offset <= 0)) { //chceme vratit cely obsah souboru / offset je mimo, vratime obsah vsech ziskanych clusteru
        to_read = file_all_clust.size();
    }
    else {
        //to_read = 
    }

    for (int i = 0; i < size; i++) {
        out.push_back(file_all_clust.at(i));

        current_byte++;
    }

    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) {
    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    if (folders_in_path.size() == 0) { //chceme ziskat root obsah, pak break
        std::vector<unsigned char> root_dir_cont = read_data_from_fat_fs(19 - 31, 14); //root slozka zacina na sektoru 19 a ma 14 sektoru (fce pricita default 31, pocita s dat. sektory; proto odecteni 31)        
        entries = retrieve_dir_items(14, root_dir_cont, true);
        std::cout << "START with size: " << entries.size() << "\n";
        for (int i = 0; i < entries.size(); i++) {
            printf("Name: %.12s", entries.at(i).file_name);
        }
        std::cout << "END with size: " << entries.size() << "\n";
        return kiv_os::NOS_Error::IO_Error;
    }
    
    //ziskani obsahu FAT tabulky pro vyhledavani sektoru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
    //nalezeni slozky ciloveho clusteru
    directory_item dir_item = retrieve_item_clust(19, fat_table1_dec, true, folders_in_path);
    int first_cluster_fol = dir_item.first_cluster; //prvni cluster slozky

    std::vector<int> folder_cluster_nums = retrieve_sectors_nums_fs(fat_table1_dec, first_cluster_fol); //nalezeni vsech clusteru slozky

    std::vector<unsigned char> one_clust_data; //data jednoho clusteru
    std::vector<unsigned char> all_clust_data; //data veskerych clusteru, na kterych je slozka rozmistena
    for (int i = 0; i < folder_cluster_nums.size(); i++) { //projdeme nactene clustery
        one_clust_data = read_data_from_fat_fs(folder_cluster_nums[i], 1); //ziskani bajtu slozky v ramci jednoho sektoru

        for (int j = 0; j < one_clust_data.size(); j++) { //vlozeni obsahu jednoho sektoru do celkoveho seznamu danych sektoru
            all_clust_data.push_back(one_clust_data.at(j));
        }
    }

    entries = retrieve_dir_items(folder_cluster_nums.size(), all_clust_data, false);
    std::cout << "START with size: " << entries.size() << "\n";
    for (int i = 0; i < entries.size(); i++) {
        printf("Name: %.12s", entries.at(i).file_name);
    }
    std::cout << "END with size: " << entries.size() << "\n";

    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::open(const char *name, uint8_t flags, uint8_t attributes, File &file) {
    // TODO implement this
    file = File {};
    file.name = const_cast<char*>(name);
    file.position = 0; //aktualni pozice, zaciname na 0

    //najit cil a ulozit do handle souboru
    int32_t target_cluster;
    
    //ziskani obsahu FAT tabulky pro vyhledavani sektoru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);

    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    
    directory_item dir_item = retrieve_item_clust(19, fat_table1_dec, false, folders_in_path); //pokus o otevreni souboru
    target_cluster = dir_item.first_cluster;
   
    if (target_cluster == -1) { //soubor nenalezen, pokus o vyhledani slozky se shodnym nazvem
        dir_item = retrieve_item_clust(19, fat_table1_dec, true, folders_in_path); //pokus o otevreni slozky
        target_cluster = dir_item.first_cluster;
    }
 
    file.handle = target_cluster;
    file.size = dir_item.filezise;

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
