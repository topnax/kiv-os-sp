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

void open_bin_floppy(const char* floppy_filename);
unsigned char hex_to_ascii(unsigned char hex_val);

int main()
{
    char floppy_filename[] = "fdos1_2_floppy.img"; //nazev souboru s disketou - fat12
    open_bin_floppy(floppy_filename);
}

/*
* Otevre soubor reprezentujici obraz diskety pro praci v binarnim rezimu.
* floppy_filename - nazev souboru reprezentujiciho virtualni disk
/**/
void open_bin_floppy(const char* floppy_filename) {
    std::ifstream floppy_img_file(floppy_filename, std::ios::binary); //nacteny img soubor s dos12
    std::streampos img_file_size; //velikost img souboru s dos12

    floppy_img_file.seekg(0, std::ios::end);
    img_file_size = floppy_img_file.tellg();
    floppy_img_file.seekg(0, std::ios::beg);

    //std::vector<unsigned char> img_file_data(img_file_size);
    //floppy_img_file.read((char*)&img_file_data[0], img_file_size);
    //std::cout << std::hex << img_file_data[0];

    //bootsector
    printf("bootsector - START\n");
    std::vector<unsigned char> bootsect_cont(SECTOR_SIZE_B);
    floppy_img_file.read((char*)&bootsect_cont[0], SECTOR_SIZE_B); //sektor 0
    for (int i = 0; i < static_cast<int>(bootsect_cont.size()); i++) {
        printf("%.2X ", bootsect_cont[i]);
    }
    printf("bootsector - END\n");

    //1. fat table
    printf("1. FAT table - START\n");
    std::vector<unsigned char> first_fat_cont(SECTOR_SIZE_B * 9);
    floppy_img_file.read((char*)&first_fat_cont[0], SECTOR_SIZE_B * 9); //sektory 1 - 9
    for (int i = 0; i < static_cast<int>(first_fat_cont.size()); i++) {
        printf("%.2X ", first_fat_cont[i]);
    }
    printf("1. FAT table - END\n");

    //2. fat table
    printf("2. FAT table - START\n");
    std::vector<unsigned char> second_fat_cont(SECTOR_SIZE_B * 9);
    floppy_img_file.read((char*)&second_fat_cont[0], SECTOR_SIZE_B * 9); //sektory 10 - 18
    for (int i = 0; i < static_cast<int>(second_fat_cont.size()); i++) {
        printf("%.2X ", second_fat_cont[i]);
    }
    printf("2. FAT table - END\n");
    

    //check shodnosti tabulek
    bool tableSame = true;
    for (int i = 0; i < static_cast<int>(second_fat_cont.size()); i++) {
        if (first_fat_cont[i] != second_fat_cont[i]) {
            tableSame = false;
        }
    }
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

    std::vector<directory_item> directory_content; //obsahuje struktury directory_item, ktere reprezentuji jednu entry ve slozce
      
    for (int i = 0; i < SECTOR_SIZE_B * 14;) {
        directory_content.resize(directory_content.size() + 1, directory_item()); //zvetseni pro pridani dalsi entry

        directory_item dir_item; //nova entry

        dir_item.filename = (char*)malloc(DIR_FILENAME_SIZE_B);
        for (int j = 0; j < DIR_FILENAME_SIZE_B; j++) { //cteni nazvu souboru
            dir_item.filename[j] = root_dir_cont[i++];
        }

        dir_item.extension = (char*)malloc(DIR_EXTENSION_SIZE_B);
        for (int j = 0; j < DIR_EXTENSION_SIZE_B; j++) { //cteni pripony
            dir_item.extension[j] = root_dir_cont[i++];
        }

        i += 15; //preskocit 15 bytu, nepotrebne info (datum vytvoreni atp.)
        printf("Before on offset %i \n", i);
        printf("Cluster: %.2X and %.2X \n", root_dir_cont[i++], root_dir_cont[i++]);
        printf("Filesize: %.2X and %.2X %.2X %.2X \n", root_dir_cont[i++], root_dir_cont[i++], root_dir_cont[i++], root_dir_cont[i++]);

        printf("After on offset %i \n", i);
        //break;

        directory_content.push_back(dir_item);


        printf("Je tam: %.8s.%3s", dir_item.filename, dir_item.extension);
    }
}

/*
* Prevede predany hex na znak (ascii).
/**/
unsigned char hex_to_ascii(unsigned char hex_val)
{
    if ('0' <= hex_val && hex_val <= '9') {
        return hex_val - '0';
    }
    else if ('a' <= hex_val && hex_val <= 'f') {
        return hex_val - 'a' + 10;
    }
    else if ('A' <= hex_val && hex_val <= 'F') {
        return hex_val - 'A' + 10;
    } else {
        return NULL;
    }
}