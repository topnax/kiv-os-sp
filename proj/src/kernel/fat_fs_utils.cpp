#include "fat_fs_utils.h"
#include "../api/hal.h"
#include "../api/api.h"

#define SECTOR_SIZE_B 512
/*
* Nacte obsah prvni fat tabulky v podobe bytu do vectoru. Prvni fat tabulka se nachazi na sektorech 1-9.
/**/
std::vector<unsigned char> load_first_fat_table_bytes() {
    std::vector<unsigned char> first_fat_cont;

    //precist sektory 1 - 9
    kiv_hal::TRegisters reg_to_read;
    kiv_hal::TDisk_Address_Packet ap_to_read;

    ap_to_read.count = 9; //prvni fat tabulka zabira 9 sektoru
    ap_to_read.sectors = (void*)malloc(SECTOR_SIZE_B * 9);
    ap_to_read.lba_index = 1; //prvni sektor fat tabulky, od ktereho cteme

    reg_to_read.rdx.l = 129; //cislo disku
    reg_to_read.rax.h = static_cast<decltype(reg_to_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_to_read.rdi.r = reinterpret_cast<decltype(reg_to_read.rdi.r)>(&ap_to_read);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_read);

    char* buffer = reinterpret_cast<char*>(ap_to_read.sectors);

    for (int i = 0; i < (SECTOR_SIZE_B * 9); i++) { //prevod na vector charu
        first_fat_cont.push_back(buffer[i]);
    }

    return first_fat_cont;
}

/*
* Nacte obsah druhe fat tabulky v podobe bytu do vectoru. Druha fat tabulka se nachazi na sektorech 10-18.
/**/
std::vector<unsigned char> load_second_fat_table_bytes() {
    std::vector<unsigned char> second_fat_cont;

    //precist sektory 10 - 18
    kiv_hal::TRegisters reg_to_read;
    kiv_hal::TDisk_Address_Packet ap_to_read;

    ap_to_read.count = 9; //druha fat tabulka zabira 9 sektoru
    ap_to_read.sectors = (void*)malloc(SECTOR_SIZE_B * 9);
    ap_to_read.lba_index = 10; //druhy sektor fat tabulky, od ktereho cteme

    reg_to_read.rdx.l = 129; //cislo disku
    reg_to_read.rax.h = static_cast<decltype(reg_to_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_to_read.rdi.r = reinterpret_cast<decltype(reg_to_read.rdi.r)>(&ap_to_read);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_read);

    char* buffer = reinterpret_cast<char*>(ap_to_read.sectors);

    for (int i = 0; i < (SECTOR_SIZE_B * 9); i++) { //prevod na vector charu
        second_fat_cont.push_back(buffer[i]);
    }

    return second_fat_cont;
}

/*
* Nacte obsah rootovske slozky v podobe bytu do vectoru. Root slozka se nachazi na sektorech 19 - 32.
/**/
std::vector<unsigned char> load_root_dir_bytes() {
    std::vector<unsigned char> root_dir_cont;

    //precist sektory 19-32
    kiv_hal::TRegisters reg_to_read;
    kiv_hal::TDisk_Address_Packet ap_to_read;

    ap_to_read.count = 14; //hlavni slozka zabira 14 sektoru
    ap_to_read.sectors = (void*)malloc(SECTOR_SIZE_B * 14);
    ap_to_read.lba_index = 19; //prvni sektor root slozky, od ktere cteme

    reg_to_read.rdx.l = 129; //cislo disku
    reg_to_read.rax.h = static_cast<decltype(reg_to_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_to_read.rdi.r = reinterpret_cast<decltype(reg_to_read.rdi.r)>(&ap_to_read);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_read);

    char* buffer = reinterpret_cast<char*>(ap_to_read.sectors);

    for (int i = 0; i < (SECTOR_SIZE_B * 14); i++) { //prevod na vector charu
        root_dir_cont.push_back(buffer[i]);
    }

    return root_dir_cont;
}

/*
* Zkontroluje konzistenci FS - FAT tabulky by mìly mít stejný obsah.
* first_table = první fat tabulka
* second_table = druhá fat tabulka
/**/
bool check_fat_table_consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table) {
    bool table_same = true;
    for (int i = 0; i < static_cast<int>(first_table.size()); i++) {
        if (first_table[i] != second_table[i]) {
            table_same = false;
            break;
        }
    }

    return table_same;
}

/*
* Prevede fat tabulku na dec, pro snazsi vyhledavani.
* fat_table_hex = obsah fat tabulky (byty)
/**/
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex) {
    std::cout << "received size is: " << fat_table_hex.size();
    
    std::vector<int> fat_table_dec;
    char convert_buffer[7]; //buffer pro prevod hex na dec
    char first_half_buffer[4]; //prvnich 12 bitu
    char second_half_buffer[4]; //zbyvajicich 12 bitu

    int actual_number;

    for (int i = 0; i < fat_table_hex.size();) {
        snprintf(convert_buffer, sizeof(convert_buffer), "%.2X%.2X%.2X", fat_table_hex[i++], fat_table_hex[i++], fat_table_hex[i++]); //obsahuje 24 bitu
        snprintf(first_half_buffer, sizeof(first_half_buffer), "%c%c%c", convert_buffer[0], convert_buffer[1], convert_buffer[2]);
        snprintf(second_half_buffer, sizeof(second_half_buffer), "%c%c%c", convert_buffer[3], convert_buffer[4], convert_buffer[5]);

        //jedno cislo tvori 12 bitu, split 3 byty
        actual_number = (int)strtol(second_half_buffer, NULL, 16);
        fat_table_dec.push_back(actual_number);
        actual_number = (int)strtol(first_half_buffer, NULL, 16);
        fat_table_dec.push_back(actual_number);
    }

    return fat_table_dec;
}

/*
* Nacte ze souboroveho systemu specifikovany pocet datovych bytu, ktere zacinaji na danem sektoru.
* start_sector_num = cislo datoveho clusteru, od ktereho ma byt cteno
* total_sector_num = celkovy pocet sektoru, ktery ma byt precten
/**/
std::vector<unsigned char> read_data_from_fat_fs(int start_sector_num, int total_sector_num) {
    std::vector<unsigned char> file_bytes(total_sector_num * SECTOR_SIZE_B); //nactene byty souboru
    std::vector<unsigned char> second_fat_cont;

    kiv_hal::TRegisters reg_to_read;
    kiv_hal::TDisk_Address_Packet ap_to_read;

    ap_to_read.count = total_sector_num; //pozadovany pocet sektoru k precteni
    ap_to_read.sectors = (void*)malloc(SECTOR_SIZE_B * total_sector_num);
    ap_to_read.lba_index = start_sector_num + 31; //sektor, od ktereho cteme (+31 presun na dat sektory)

    reg_to_read.rdx.l = 129; //cislo disku
    reg_to_read.rax.h = static_cast<decltype(reg_to_read.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    reg_to_read.rdi.r = reinterpret_cast<decltype(reg_to_read.rdi.r)>(&ap_to_read);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_read);
    char* buffer = reinterpret_cast<char*>(ap_to_read.sectors);

    for (int i = 0; i < (total_sector_num * SECTOR_SIZE_B); i++) { //prevod na vector charu
        file_bytes.push_back(buffer[i]);
    }

    return file_bytes;
}

/*
* Vrati objekt std::vector<kiv_os::TDir_Entry> reprezentujici obsah pozadovane slozky (polozky). Soubory / podslozky.
* num_sectors - pocet sektoru, ktery je obsazen slozkou
* dir_clusters - obsah clusteru, ze kterych ma byt obsah slozky vycten
/**/
std::vector<kiv_os::TDir_Entry> retrieve_dir_items(int num_sectors, std::vector<unsigned char> dir_clusters) {
    std::vector<kiv_os::TDir_Entry> directory_content; //obsahuje struktury TDir_Entry, ktere reprezentuji jednu entry ve slozce

    char first_clust_buff_conv[5]; //buffer pro konverzi dvou hexu na dec (reprezentuje prvni cluster souboru)
    char filesize_buff_conv[9]; //buffer pro konverzi ctyr hexu na dec (reprezentuje velikost souboru)

    for (int i = 0; i < SECTOR_SIZE_B * num_sectors;) {
        if (dir_clusters[i] == 0) { /* pokud nazev zacina 0, pak neni polozka obsazena -> ani zadna dalsi, cely obsah slozky uz je vypsan */
            break;
        }

        kiv_os::TDir_Entry dir_item; //nova entry

        int j = 0;
        bool end_encountered = false; //true pokud narazime na konec nazvu (fatka ma fix na 8 byt)
        while (j < DIR_FILENAME_SIZE_B && !end_encountered) { //cteni nazvu souboru
            if ((int)dir_clusters[i] == 32) { //konec nazvu souboru (fix na 8 byt)
                end_encountered = true;
                i += DIR_FILENAME_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 8 byt)
            }
            else {
                dir_item.file_name[j] = dir_clusters[i++];
                std::cout << "found: " << dir_item.file_name[j];
                j++;
            }
        }

        int position_filename = j; //aktualni pozice ukladani ve stringu file_name

        dir_item.file_name[position_filename] = '.';
        position_filename++;

        j = 0;
        end_encountered = false;
    
        while (j < DIR_EXTENSION_SIZE_B && !end_encountered) { //cteni pripony
            if ((int)dir_clusters[i] == 32) { //konec pripony souboru (fix na 3 byt)
                end_encountered = true;
                i += DIR_EXTENSION_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 3 byt)
            }
            else {
                dir_item.file_name[position_filename] = dir_clusters[i++];
                position_filename++;
                j++;
            }
        }

        if (dir_item.file_name[position_filename - 1] == '.') { //pokud konci teckou, pak nebyla zadna pripona, odstranit tecku
            dir_item.file_name[position_filename - 1] = '\0';
        }else if (position_filename != 12) { //využita plna kapacita, dostali bychom se mimo rozsah
            dir_item.file_name[position_filename] = '\0';
        }

        i += 15; //preskocit 15 bytu, nepotrebne info (datum vytvoreni atp.)

        snprintf(first_clust_buff_conv, sizeof(first_clust_buff_conv), "%.2X%.2X", dir_clusters[i++], dir_clusters[i++]);
        //dir_item.first_cluster = (int)strtol(first_clust_buff_conv, NULL, 16);
        snprintf(filesize_buff_conv, sizeof(filesize_buff_conv), "%.2X%.2X%.2X%.2X", dir_clusters[i++], dir_clusters[i++], dir_clusters[i++], dir_clusters[i++]);
        //dir_item.filezise = (int)strtol(filesize_buff_conv, NULL, 16);

        directory_content.push_back(dir_item);
    }

    return directory_content;
}