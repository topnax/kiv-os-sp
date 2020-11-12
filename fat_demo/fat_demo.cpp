// fat_demo.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//
#define SECTOR_SIZE_B 512
#define DIR_FILENAME_SIZE_B 8
#define DIR_EXTENSION_SIZE_B 3
#define DIR_FIRST_CLUST_SIZE_B 2
#define DIR_FILESIZE_B 4

#include <iostream>
#include <fstream>
#include <vector>
#include "def_fs_structs.h"

std::ifstream open_bin_floppy(const char* floppy_filename);
std::vector<unsigned char> load_bootsect_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_first_fat_table_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_second_fat_table_bytes(std::ifstream& floppy_img_file);
bool check_fat_table_consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);
//std::vector<unsigned char> load_root_dir(std::ifstream& floppy_img_file);
//void seek_data_area(int start_addr, int end_addr)

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

    //root directory
    printf("root dir - START\n");
    std::vector<unsigned char> root_dir_cont(SECTOR_SIZE_B * 14);
    floppy_img_file.read((char*)&root_dir_cont[0], SECTOR_SIZE_B * 14); //sektory 19 - 32
    for (int i = 0; i < static_cast<int>(root_dir_cont.size()); i++) {
        printf("%.2X ", root_dir_cont[i]);
    }
    printf("root dir - END\n");

    std::vector<directory_item> directory_content = get_dir_items(14, root_dir_cont); //ziskani obsahu slozky (struktury directory_item, ktere reprezentuji jednu entry ve slozce)

    for (int i = 0; i < directory_content.size(); i++) {
        directory_item dir_item = directory_content.at(i);
        printf("Obsah: %s.%s, velikost %d, prvni cluster %d\n", dir_item.filename, dir_item.extension, dir_item.filezise, dir_item.first_cluster);
    }
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