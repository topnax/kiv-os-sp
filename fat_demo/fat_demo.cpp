#define SECTOR_SIZE_B 512
#define DIR_FILENAME_SIZE_B 8
#define DIR_EXTENSION_SIZE_B 3
#define DIR_FIRST_CLUST_SIZE_B 2
#define DIR_FILESIZE_B 4

#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

#include "def_fs_structs.h"

std::ifstream open_bin_floppy(const char* floppy_filename);
std::vector<unsigned char> load_bootsect_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_first_fat_table_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_second_fat_table_bytes(std::ifstream& floppy_img_file);
bool check_fat_table_consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);
std::vector<int> retrieve_sectors_nums_fs(std::vector<int> fat_table_dec, int starting_sector);
std::vector<unsigned char> read_from_fs(std::ifstream& floppy_img_file, int sector_num, int size_to_read);
std::vector<unsigned char> load_root_dir_bytes(std::ifstream& floppy_img_file);
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex);
void seek_data_area(int start_addr, int end_addr);

std::vector<directory_item> get_dir_items(int num_sectors, std::vector<unsigned char> root_dir_cont);

int main()
{
    char floppy_filename[] = "fdos1_2_floppy.img"; //nazev souboru s disketou - fat12
    std::ifstream floppy_img_file = open_bin_floppy(floppy_filename);

    std::vector<unsigned char> bootsect_cont = load_bootsect_bytes(floppy_img_file); //bootsector - byt
    std::vector<unsigned char> first_fat_cont = load_first_fat_table_bytes(floppy_img_file); //prvni fat tab - byt
    std::vector<unsigned char> second_fat_cont = load_second_fat_table_bytes(floppy_img_file); //druha fat tab - byt

    //check shodnosti tabulek
    bool tableSame = check_fat_table_consistency(first_fat_cont, second_fat_cont);
    if (tableSame) {
        printf("FAT tabulky jsou shodne!!");
    }
    else {
        printf("FAT tabulky jsou odlisne!!");
    }

    std::vector<unsigned char> root_dir_cont = load_root_dir_bytes(floppy_img_file); //obsah root slozky - byt
    std::vector<directory_item> directory_content = get_dir_items(14, root_dir_cont); //ziskani obsahu slozky (struktury directory_item, ktere reprezentuji jednu entry ve slozce)

    for (int i = 0; i < directory_content.size(); i++) {
        directory_item dir_item = directory_content.at(i);
        printf("Obsah: %s.%s, velikost %d, prvni cluster %d\n", dir_item.filename, dir_item.extension, dir_item.filezise, dir_item.first_cluster);
    }

    std::vector<int> fat_table_dec = convert_fat_table_to_dec(first_fat_cont); //prevod fat tabulky z hex formatu na dec, pro snazsi vyhledavani
    
    printf("Printing AUTOEXEC.BAT - START\n");
    std::vector<int> sectors_nums_data = retrieve_sectors_nums_fs(fat_table_dec, 2579);

    //nacteni dat ze ziskaneho seznamu sektoru
    int bytes_read = 0;
    std::vector<unsigned char> retrieved_data;

    int i = 0;
    for (; i < sectors_nums_data.size() - 1; i++) { //krome posledniho cist cely sektor
        retrieved_data = read_from_fs(floppy_img_file, sectors_nums_data[i], SECTOR_SIZE_B);

        bytes_read += SECTOR_SIZE_B;
        for (int j = 0; j < SECTOR_SIZE_B; j++) {
            printf("%c", retrieved_data[j]);
        }
    }

    retrieved_data = read_from_fs(floppy_img_file, sectors_nums_data[i], 1870 - bytes_read); //zbytek bytu, nebude se cist cely sektor
    for (int j = 0; j < 1870 - bytes_read; j++) {
        printf("%c", retrieved_data[j]);
    }
    printf("Printing AUTOEXEC.BAT - END\n");
}

/*
* Nacte obsah bootsectoru v podobe bytu do vectoru. Bootsector se nachazi na zacatku fs fat12 - sektor 0.
* floppy_img_file = ifstream objekt s daty floppiny
/**/
std::vector<unsigned char> load_bootsect_bytes(std::ifstream& floppy_img_file) {
    std::vector<unsigned char> bootsect_cont(SECTOR_SIZE_B);

    floppy_img_file.seekg(0, std::ios::beg);
    floppy_img_file.read((char*)&bootsect_cont[0], SECTOR_SIZE_B); //sektor 0

    return bootsect_cont;
}

/*
* Nacte obsah prvni fat tabulky v podobe bytu do vectoru. Prvni fat tabulka se nachazi na sektorech 1-9.
* floppy_img_file = ifstream objekt s daty floppiny
/**/
std::vector<unsigned char> load_first_fat_table_bytes(std::ifstream& floppy_img_file) {
    std::vector<unsigned char> first_fat_cont(SECTOR_SIZE_B * 9);

    floppy_img_file.seekg(SECTOR_SIZE_B, std::ios::beg);
    floppy_img_file.read((char*)&first_fat_cont[0], SECTOR_SIZE_B * 9); //sektory 1 - 9
   
    return first_fat_cont;
}

/*
* Nacte obsah druhe fat tabulky v podobe bytu do vectoru. Druha fat tabulka se nachazi na sektorech 10-18.
* floppy_img_file = ifstream objekt s daty floppiny
/**/
std::vector<unsigned char> load_second_fat_table_bytes(std::ifstream& floppy_img_file) {
    std::vector<unsigned char> second_fat_cont(SECTOR_SIZE_B * 9);

    floppy_img_file.seekg(SECTOR_SIZE_B * 10, std::ios::beg);
    floppy_img_file.read((char*)&second_fat_cont[0], SECTOR_SIZE_B * 9); //sektory 10 - 18

    return second_fat_cont;
}

/*
* Prevede fat tabulku na dec, pro snazsi vyhledavani.
* fat_table_hex = obsah fat tabulky (byty)
/**/
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex) {
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
* Zkontroluje konzistenci FS - FAT tabulky by měly mít stejný obsah. 
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
* Otevre soubor reprezentujici obraz diskety pro praci v binarnim rezimu.
* floppy_filename - nazev souboru reprezentujiciho virtualni disk
/**/
std::ifstream open_bin_floppy(const char* floppy_filename) {
    std::ifstream floppy_img_file(floppy_filename, std::ios::binary); //nacteny img soubor s dos12
   
    return floppy_img_file;
}

/*
* Vytvori vector obsahujici objekty directory_item reprezentujici obsah slozky.
* num_sectors = pocet sektoru vyhrazenych pro obsah dane slozky (root slozka ma 14 sektoru)
* root_dir_cont = vector obsahujici byty reprezentujici jednu slozku
/**/
std::vector<directory_item> get_dir_items(int num_sectors, std::vector<unsigned char> root_dir_cont) {
    std::vector<directory_item> directory_content; //obsahuje struktury directory_item, ktere reprezentuji jednu entry ve slozce

    char first_clust_buff_conv[5]; //buffer pro konverzi dvou hexu na dec (reprezentuje prvni cluster souboru)
    char filesize_buff_conv[9]; //buffer pro konverzi ctyr hexu na dec (reprezentuje velikost souboru)

    for (int i = 0; i < SECTOR_SIZE_B * num_sectors;) {
        if (root_dir_cont[i] == 0) { /* pokud nazev zacina 0, pak neni polozka obsazena -> ani zadna dalsi, cely obsah slozky uz je vypsan */
            break;
        }

        directory_item dir_item; //nova entry

        dir_item.filename = (char*)malloc(DIR_FILENAME_SIZE_B + 1);

        int j = 0;
        bool end_encountered = false; //true pokud narazime na konec nazvu (fatka ma fix na 8 byt)
        while (j < DIR_FILENAME_SIZE_B && !end_encountered) { //cteni nazvu souboru
            if ((int)root_dir_cont[i] == 32) { //konec nazvu souboru (fix na 8 byt)
                end_encountered = true;
                i += DIR_FILENAME_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 8 byt)
            }
            else {
                dir_item.filename[j] = root_dir_cont[i++];
                j++;
            }
        }
        dir_item.filename[j] = '\0';

        j = 0;
        end_encountered = false;
        dir_item.extension = (char*)malloc(DIR_EXTENSION_SIZE_B + 1);
        while (j < DIR_EXTENSION_SIZE_B && !end_encountered) { //cteni pripony
            if ((int)root_dir_cont[i] == 32) { //konec pripony souboru (fix na 3 byt)
                end_encountered = true;
                i += DIR_EXTENSION_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 3 byt)
            }
            else {
                dir_item.extension[j] = root_dir_cont[i++];
                j++;
            }
        }
        dir_item.extension[j] = '\0';

        i += 15; //preskocit 15 bytu, nepotrebne info (datum vytvoreni atp.)

        snprintf(first_clust_buff_conv, sizeof(first_clust_buff_conv), "%.2X%.2X", root_dir_cont[i++], root_dir_cont[i++]);
        dir_item.first_cluster = (int)strtol(first_clust_buff_conv, NULL, 16);
        snprintf(filesize_buff_conv, sizeof(filesize_buff_conv), "%.2X%.2X%.2X%.2X", root_dir_cont[i++], root_dir_cont[i++], root_dir_cont[i++], root_dir_cont[i++]);
        dir_item.filezise = (int)strtol(filesize_buff_conv, NULL, 16);

        directory_content.push_back(dir_item);
    }

    return directory_content;
}

/*
* Nacte obsah rootovske slozky v podobe bytu do vectoru. Root slozka se nachazi na sektorech 19 - 32.
* floppy_img_file = ifstream objekt s daty floppiny
/**/
std::vector<unsigned char> load_root_dir_bytes(std::ifstream& floppy_img_file) {
    std::vector<unsigned char> root_dir_cont(SECTOR_SIZE_B * 14);

    floppy_img_file.seekg(SECTOR_SIZE_B * 19, std::ios::beg);
    floppy_img_file.read((char*)&root_dir_cont[0], SECTOR_SIZE_B * 14); //sektory 19 - 32

    return root_dir_cont;
}

/*
* Vrati seznam sektoru na floppyne, ktere obsazuje specifikovany soubor.
* fat_table_dec = fat tabulka, dle ktere dojde k zisani informaci o lokaci souboru (v dec formatu)
* starting_sector = prvni sektor souboru
/**/
std::vector<int> retrieve_sectors_nums_fs(std::vector<int> fat_table_dec, int starting_sector) {
    std::vector<int> sector_list; //seznam sektorů, na kterých se data souboru nachází

    int cluster_num = -1;

    int traversed_sector = starting_sector;
    while (traversed_sector < 4088 || traversed_sector > 4095) {//dokud se nejedna o posledni cluster souboru - rozmezi 0xFF8-0xFFF = posledni sektor souboru
        if (sector_list.size() == 0) { //startovni pridat do seznamu
            sector_list.push_back(traversed_sector);

            traversed_sector = fat_table_dec[traversed_sector]; //prechod na dalsi sektor pomoci fat tabulky
            continue;
        }

        sector_list.push_back(traversed_sector);
        traversed_sector = fat_table_dec[traversed_sector];
    }

    printf("Print bytes - start");
    for (int i = 0; i < sector_list.size(); i++) {
        printf("%d \n", sector_list[i]);
    }
    printf("\n");
    printf("Print bytes - end");

    return sector_list;
}

/*
* Nacte ze souboroveho systemu spicifikovany pocet bytu, ktere zacinaji na danem sektoru.
* floppy_img_file = ifstream objekt s daty floppiny
* sector_num = cislo clusteru, od ktereho ma byt cteno
* size_to_read = pocet bytu, ktery ma byt precten
/**/
std::vector<unsigned char> read_from_fs(std::ifstream& floppy_img_file, int sector_num, int size_to_read) {
    std::vector<unsigned char> file_bytes(size_to_read); //nactene byty souboru

    floppy_img_file.seekg((sector_num + 31) * SECTOR_SIZE_B, std::ios::beg);
    floppy_img_file.read((char*)&file_bytes[0], size_to_read);

    return file_bytes;
}