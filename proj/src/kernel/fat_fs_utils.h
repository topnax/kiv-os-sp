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

struct directory_item {
	std::string filename; //nazev souboru
	std::string extension; //pripona souboru
	int filezise; //velikost souboru (byt)
	int first_cluster; //prvni cluster souboru
};

std::vector<unsigned char> load_first_fat_table_bytes();
std::vector<unsigned char> load_second_fat_table_bytes();
std::vector<unsigned char> load_root_dir_bytes();
bool check_fat_table_consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);
std::vector<int> convert_fat_table_to_dec(std::vector<unsigned char> fat_table_hex);
std::vector<unsigned char> read_data_from_fat_fs(int start_sector_num, int total_sector_num);
void write_data_to_fat_fs(int start_sector_num, std::vector<char> buffer_to_write);
std::vector<kiv_os::TDir_Entry> retrieve_dir_items(int num_sectors, std::vector<unsigned char> dir_clusters, bool is_root);
directory_item retrieve_item_clust(int start_cluster, std::vector<int> fat_table_dec, bool is_folder, std::vector<std::string> path);
std::vector<directory_item> retrieve_folders_cur_folder(std::vector<int> fat_table_dec, int working_dir_sector);
std::vector<directory_item> get_dir_items(int num_sectors, std::vector<unsigned char> dir_cont);
std::vector<int> retrieve_sectors_nums_fs(std::vector<int> fat_table_dec, int starting_sector);
std::vector<std::string> path_to_indiv_items(const char *path_file);
int retrieve_free_byte_count(int sector_num);
int write_folder_to_fs(int newly_created_fol_clust, std::string newly_created_fol_name, int upper_fol_clust_first, int upper_fol_clust_last, int upper_fol_clust_last_free);
int retrieve_free_cluster_index(std::vector<int> fat_table_dec);
unsigned char conv_char_arr_to_hex(char char_arr[2]);
std::vector<unsigned char> convert_num_to_bytes_fat(int target_index, std::vector<unsigned char> fat_table_hex, int num_to_inject);
void save_fat_tables(std::vector<unsigned char> fat_table);
std::vector<unsigned char> convert_dec_num_to_hex(long num_dec);
void update_size_file_in_folder(char* filename_path, int offset, int original_size, int newly_written_bytes, std::vector<int> fat_table_dec);
int create_folder(const char* folder_path, std::vector<int> fat_table_dec);