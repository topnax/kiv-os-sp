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
    //rozdeleni na jednotlive polozky v ceste - START
    std::vector<std::string> folders_in_path; //vector obsahuje veskere slozky v absolutni ceste

    std::vector<char> one_item; //jedna slozka / polozka v absolutni ceste
    char character = 'A'; //jeden znak v ceste
    int counter = 0;

    while (true) { //dokud nedojdeme na konec stringu
        character = file.name[counter];
        
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

    //zaciname v root slozce
    //precist sektory 19-32
    //read test
    kiv_hal::TRegisters reg_to_read;
    kiv_hal::TDisk_Address_Packet ap_to_read;

    ap_to_read.count = 14; //hlavni slozka zabira 14 sektoru
    ap_to_read.sectors = (void*)malloc(disk_parameters.bytes_per_sector * 14);
    ap_to_read.lba_index = 19; //prvni sektor root slozky, od ktere cteme

    reg_to_read.rdx.l = 129; //cislo disku
    reg_to_read.rax.h = static_cast<decltype(reg_to_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_to_read.rdi.r = reinterpret_cast<decltype(reg_to_read.rdi.r)>(&ap_to_read);

    std::cout << "before read";
    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_read);
    std::cout << "after read";
    //ap_read.sectors = reinterpret_cast<decltype(reg_read.rdi.r)>(&ap);

    char* buffer_root = reinterpret_cast<char*>(ap_to_read.sectors);
    size_t buffer_root_length = reg_to_read.rcx.r;
    std::cout << "Conten is: " << buffer_root[8 + 25];

    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<unsigned char> fat_table2_hex = load_second_fat_table_bytes();
    bool table_same = check_fat_table_consistency(fat_table1_hex, fat_table2_hex);

    if (table_same) {
        std::cout << "Fat tabulky jsou shodne!";
    }
    else {
        std::cout << "Fat tabulky jsou odlisne!";
    }

    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
    for (int i = 0; i < fat_table1_dec.size(); i++) {
        //printf("table on index %i got: %i\n", i, fat_table1_dec[i]);
    }
    //nacteni obsahu root slozky - START
    std::vector<unsigned char> root_dir_cont = load_root_dir_bytes();
    std::vector<kiv_os::TDir_Entry> root_entries = retrieve_dir_items(14, root_dir_cont);

    for (int i = 0; i < root_entries.size(); i++) {
        printf("%.12s \n", root_entries.at(i).file_name);
    }

    for (int i = 0; i < root_dir_cont.size(); i++) {
        //printf("%c", root_dir_cont.at(i));
    }
    //nacteni obsahu root slozky - KONEC
        

    for (int i = 0; i < folders_in_path.size(); i++) {
        //std::cout << "here is: " << folders_in_path.at(i);
    }

    // type example.txt
    printf("tried to read from FAT FS, disk_num=%d, path=%s, size=%d, offset=%d\n", disk_number, file.name, size, offset);
    std::vector<unsigned char> retrieved_bytes = read_data_from_fat_fs(2248, 3);

    printf("Got: - STRART");
    for (int i = 0; i < retrieved_bytes.size(); i++) {
        //std::cout << retrieved_bytes.at(i);
    }
    printf("Got: - END");

    directory_item target = retrieve_item_cluster(19, fat_table1_dec, folders_in_path);

    //read test
    kiv_hal::TRegisters reg_read;
    kiv_hal::TDisk_Address_Packet ap_read;

    ap_read.count = 2; //pocet sektoru k precteni
    ap_read.sectors = (void*) malloc(size * 2);
    std::cout << "mallocated: " << size * 2;

    ap_read.lba_index = 2500; //adresa prvniho sektoru na disku

    reg_read.rdx.l = 129; //cislo disku
    reg_read.rax.h = static_cast<decltype(reg_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_read.rdi.r = reinterpret_cast<decltype(reg_read.rdi.r)>(&ap_read);

    std::cout << "before read";
    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_read);
    std::cout << "after read";
    //ap_read.sectors = reinterpret_cast<decltype(reg_read.rdi.r)>(&ap);

    char* buffer = reinterpret_cast<char*>(ap_read.sectors);
    size_t buffer_length = reg_read.rcx.r;
    //std::cout << "content isSSSS:" << buffer;
    
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
