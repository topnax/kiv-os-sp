#pragma once

#define SECTOR_SIZE_B 512
#define DIR_FILENAME_SIZE_B 8
#define DIR_EXTENSION_SIZE_B 3
#define DIR_FIRST_CLUST_SIZE_B 2
#define DIR_FILESIZE_B 4

#include "../api/hal.h"
#include "../api/api.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <sstream>
#include <iterator>
#include <algorithm>

std::vector<unsigned char> load_first_fat_table_bytes();
std::vector<unsigned char> load_second_fat_table_bytes();
std::vector<unsigned char> load_root_dir_bytes();
bool check_fat_table_consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex);
std::vector<unsigned char> read_data_from_fat_fs(int start_sector_num, int total_sector_num);
std::vector<kiv_os::TDir_Entry> retrieve_dir_items(int num_sectors, std::vector<unsigned char> dir_clusters);