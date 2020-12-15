#pragma once

#define SECTOR_SIZE_B 512
#define DIR_FILENAME_SIZE_B 8
#define DIR_EXTENSION_SIZE_B 3
#define DIR_FIRST_CLUST_SIZE_B 2
#define DIR_FILESIZE_B 4

#include "api.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <sstream>
#include <iterator>
#include <algorithm>

std::vector<unsigned char> load_bootsect_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_first_fat_table_bytes(std::ifstream& floppy_img_file);
std::vector<unsigned char> load_second_fat_table_bytes(std::ifstream& floppy_img_file);
std::vector<int> retrieve_sectors_nums_fs(std::vector<int> fat_table_dec, int starting_sector);
std::vector<unsigned char> read_from_fs(std::ifstream& floppy_img_file, int sector_num, int size_to_read);
std::vector<unsigned char> load_root_dir_bytes(std::ifstream& floppy_img_file);
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex);
std::vector<directory_item> get_dir_items(int num_sectors, std::vector<unsigned char> root_dir_cont);
std::vector<directory_item> retrieve_folders_cur_folder(int working_dir_sector);
directory_item retrieve_dir_items(int start_sector, std::vector<std::string> path);
directory_item retrieve_file_dir_item(int start_clust_folder, std::string filename);
int retrieve_free_byte_count(int sector_num);
int write_folder_to_fs(int newly_created_fol_clust, std::string newly_created_fol_name, int upper_fol_clust_first, int upper_fol_clust_last, int upper_fol_clust_last_free);