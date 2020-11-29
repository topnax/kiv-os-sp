#include "fat_fs_utils.h"

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