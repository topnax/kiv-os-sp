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
    //ziskani obsahu FAT tabulky pro vyhledavani sektoru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
    
    std::vector<int> file_clust_nums = retrieve_sectors_nums_fs(fat_table1_dec, file.handle); //ziskani seznamu clusteru, na kterych se soubor nachazi

    std::vector<unsigned char> file_one_clust; //obsah jednoho clusteru souboru
    std::vector<unsigned char> file_all_clust; //obsah veskerych clusteru souboru

    for (int i = 0; i < file_clust_nums.size(); i++) { //pruchod vsemi clustery, pres ktere je soubor natazen
        file_one_clust = read_data_from_fat_fs(file_clust_nums[i], 1); //ziskani bajtu souboru v ramci jednoho sektoru

        for (int j = 0; j < file_one_clust.size(); j++) { //vlozeni obsahu jednoho sektoru do celkoveho seznamu bajtu
            file_all_clust.push_back(file_one_clust.at(j));
        }
    }

    //mame cely obsah souboru - vratit jen pozadovanou cast
    int to_read = -1;

    if ((offset + size) > file.size) { //offset je mimo, vratit rozmezi offset - konec souboru
        for (int i = offset; i < file.size; i++) {
            out.push_back(file_all_clust.at(i));
        }
    }
    else { //rozsah ok
        to_read = size;

        for (int i = 0; i < to_read; i++) {
            out.push_back(file_all_clust.at(offset + i));
        }
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
    //rozdelit na jednotlive slozky cesty => n polozek, hledame cluster n - 1 ; posledni prvek (n) bude nazev nove vytvarene slozky
    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    std::string new_folder_name = folders_in_path.at(folders_in_path.size() - 1); //nazev nove slozky
        
    folders_in_path.pop_back(); //posledni polozka v seznamu je nazev nove slozky, tu ted nehledame

    //FAT tabulka nutna pro vyhledani ciloveho clusteru
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);

    directory_item target_folder = retrieve_item_clust(19, fat_table1_dec, true, folders_in_path); //nalezeni ciloveho clusteru (slozka n-1)
    //ted by se kontrolovalo, zda uz neexistuje soubor/slozka se stejnym nazvem - ale taky to muze resit open jeste pred zavolanim

    //zjisteni, zda je ve slozce volne misto pro dalsi podslozku - START
    //sektor = 512b - muze obsahovat 16 slozek => jedna slozka 32b
    std::vector<int> sectors_nums_data = retrieve_sectors_nums_fs(fat_table1_dec, target_folder.first_cluster); //zjistime sektory, na kterych se slozka nachazi
    std::vector<unsigned char> retrieved_data_clust; //obsahuje data jednoho clusteru
    std::vector<directory_item> directory_content; //slozky + soubory na jednom clusteru
    std::vector<directory_item> all_dir_items; //obsahuje veskere podslozky i soubory ve slozce (pres vsechny clustery...)

    for (int i = 0; i < sectors_nums_data.size(); i++) { //projdeme nactene clustery
        retrieved_data_clust = read_data_from_fat_fs(sectors_nums_data[i], 1); //ziskani bajtu slozky v ramci jednoho sektoru
        directory_content = get_dir_items(1, retrieved_data_clust); //prevod na struktury, kazda reprez. jednu slozku

        for (int i = 0; i < directory_content.size(); i++) { //pridani do seznamu
            directory_item dir_item = directory_content.at(i);
            all_dir_items.push_back(dir_item);
        }
    }

    std::cout << "Items in target folder - start";
    for (int i = 0; i < all_dir_items.size(); i++) {
        std::cout << all_dir_items.at(i).filename;
    }
    std::cout << "Items in target folder - end";

    //projdu posledni sektor, na kterem je slozka umistena a zjistim pocet volnych bytu => zjisteni, jestli pro vytvoreni nove slozky potreba alokovat novy cluster ci ne
    int free_bytes = retrieve_free_byte_count(sectors_nums_data.at(sectors_nums_data.size() - 1));
    if (free_bytes < 32) { //jedna entry zabira 32 bytu - pokud neni volne misto, pak alokace dalsiho clusteru
        std::cout << "V danem clusteru uz neni misto";
    }
    std::cout << "Zbyva volneho mista: " << free_bytes << "\n";
    //zjisteni, zda je ve slozce volne misto pro dalsi podslozku - KONEC

     //nalezt volne misto ve fat tabulce pro novou slozku - START
    int free_cluster_new_folder = -1; //index volneho clusteru - na nej ulozit 4095 - v hex FFF, znaci posledni cluster souboru / slozky

    for (int i = 0; i < fat_table1_dec.size(); i++) {
        if (fat_table1_dec[i] == 0) { //nepouzivany cluster, v hex 0
            free_cluster_new_folder = i;
            break;
        }
    }

    if (free_cluster_new_folder == -1) { //cluster pro novou slozku se nepodarilo najit
        std::cout << "Na disku jiz neni misto - slozku nelze vytvorit.";
        return kiv_os::NOS_Error::IO_Error;
    }
    //nalezt volne misto ve fat tabulce pro novou slozku - KONEC
    std::vector<char> buffer_to_write;
    std::string text = "Duis ante orci, molestie vitae vehicula venenatis, tincidunt ac pede.Aliquam erat volutpat.Maecenas aliquet accumsan leo.Curabitur ligula sapien, pulvinar a vestibulum quis, facilisis vel sapien.Praesent vitae arcu tempor neque lacinia pretium.Lorem ipsum dolor sit amet, consectetuer adipiscing elit.Curabitur bibendum justo non orci.Lorem ipsum dolor sit amet, consectetuer adipiscing elit.Aenean vel massa quis mauris vehicula lacinia.Etiam ligula pede, sagittis quis, interdum ultricies, scelerisque eu.Duis risus.In dapibus augue non sapien.Duis ante orci, molestie vitae vehicula venenatis, tincidunt ac pede.Nullam eget nisl.Etiam ligula pede, sagittis quis, interdum ultricies, scelerisque eu.Etiam posuere lacus quis dolor.Etiam neque.Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos hymenaeos.Sed elit dui, pellentesque a, faucibus vel, interdum nec, diam.huiozhuiozizuiztuigzurghetfzevrtcxtzvrdchzdfvhtjrvtzrvtzrugtzrtzrvtzrtztrtzirtzifhgfufuzzufuzfuxcbhvxcghvsexycaj";//12212

    for (int i = 0; i < text.size(); i++) {
        buffer_to_write.push_back(text.at(i));
    }

    write_data_to_fat_fs(50, buffer_to_write); //-31
    //write_folder_to_fs(free_cluster_new_folder, new_folder_name, sectors_nums_data.at(0), sectors_nums_data.at(sectors_nums_data.size() - 1), free_bytes);
   
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::rmdir(const char *name) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::write(File file, std::vector<char> buffer, size_t size, size_t offset, size_t &written) {
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
    for (int i = 0; i < fat_table1_dec.size(); i++) {
        printf("Fat on index %d got %d\n", i, fat_table1_dec.at(i));
    }

    std::vector<int> file_clust_nums = retrieve_sectors_nums_fs(fat_table1_dec, file.handle); //seznam clusteru, na kterych se soubor nachazi

    int cluster_fully_occ_bef = (offset) / SECTOR_SIZE_B; //pocet plne obsazenych clusteru pred offsetem
    int bytes_to_save = (offset) - (cluster_fully_occ_bef * SECTOR_SIZE_B);
        
    int sector_num; //poradi sektoru, na ktery ma byt zapsano ; zaokrouhleno nahoru
    
    if (offset == 0) {
        sector_num = 1;
    }
    else {
        sector_num = offset / SECTOR_SIZE_B + (offset % SECTOR_SIZE_B != 0);
    }

    std::cout << "about to write on sector num: " << sector_num << "\n";

    int sector_num_vect = file_clust_nums.at(sector_num - 1); //nalezeni odpovidajiciho sektoru v poradi v ramci vektoru

    int bytes_to_save_clust = offset % SECTOR_SIZE_B; //pocet bajtu, ktere jsou na clusteru, na ktery se bude zapisovat pred offsetem (ty chceme uchovat)

    std::vector<unsigned char> data_last_clust = read_data_from_fat_fs(sector_num_vect, 1);
    std::vector<unsigned char> data_to_write;

    for (int i = 0; i < bytes_to_save_clust; i++) { //pruchod bajtu pred offsetem na danem clusteru
        data_to_write.push_back(data_last_clust.at(i));
    }

    for (int i = 0; i < buffer.size(); i++) { //pridani obsahu bufferu pro zapis
        data_to_write.push_back(buffer.at(i));
    }

    std::cout << "to write is: " << data_to_write.size();
    std::string content_clust;

    for (int i = 0; i < data_to_write.size(); i++) {
        content_clust.push_back(data_to_write.at(i));
    }

    //v bufferu je ulozen obsah vsech clusteru, ktere maji byt prepsany - zaciname od clusteru sector_num_vect, pripadne pak posun na dalsi, pokud buffer > 512 - teoreticky zapis na vice clusteru
    int clusters_count = data_to_write.size() / SECTOR_SIZE_B + (data_to_write.size() % SECTOR_SIZE_B != 0); //pocet clusteru, pres ktere bude buffer ulozen
    std::cout << "cluster count is: " << clusters_count;

    std::vector<char> clust_data_write; //data pro zapis do jednoho clusteru
    for (int i = 0; i < clusters_count; i++) {
        if (i == (clusters_count - 1)) { //posledni cluster, nemusi byt vyuzity cely
            for (int j = 0; j < data_to_write.size() - (i * SECTOR_SIZE_B); j++) {
                clust_data_write.push_back(data_to_write.at(j + (i * SECTOR_SIZE_B)));
            }

            std::cout << "last cluster size is: " << clust_data_write.size();
        }
        else { //ćluster, ktery neni posledni bude vyuzit cely
            for (int j = 0; j < SECTOR_SIZE_B; j++) {
                clust_data_write.push_back(data_to_write.at(j + (i * SECTOR_SIZE_B)));
            }

            std::cout << "other cluster size is: " << clust_data_write.size();
        }

        
        if (sector_num - 1 + i < file_clust_nums.size()) { //ok, vejdeme se do alokovanych clusteru
            write_data_to_fat_fs(file_clust_nums.at(sector_num - 1 + i), clust_data_write); //zapis dat do fs, postupne posun po alokovanych clusterech

            int free_clust_index = retrieve_free_cluster_index(fat_table1_dec);
            std::cout << "got free clust: " << free_clust_index;

            //ok, mame novy cluster, prirazeni v dec tabulce
            fat_table1_dec.at(file_clust_nums.at(file_clust_nums.size() - 1)) = free_clust_index;
            fat_table1_dec.at(free_clust_index) = 4095;

            for (int i = 0; i < fat_table1_dec.size(); i++) {
               // printf("Fat on index %d got %d\n", i, fat_table1_dec.at(i));
            }
            std::cout << "Writing to: " << file_clust_nums.at(file_clust_nums.size() - 1);

            int index_to_edit = file_clust_nums.at(file_clust_nums.size() - 1);
            index_to_edit = 1;

            //v hex tabulkach upravit cislo na indexu nove prideleneho clusteru a posledniho clusteru puvodne prideleno souboru
            if (index_to_edit % 2 == 0) { //sudy index
                int first_index_hex_tab = index_to_edit * 1.5; //index volneho clusteru v hex(na dvou bajtech)

                char free_cluster_index_first = fat_table1_hex.at(first_index_hex_tab);
                char free_cluster_index_sec = fat_table1_hex.at(first_index_hex_tab + 1);

                //v hex tabulce je ulozeno na indexu first_index_hex_tab a first_index_hex_tab + 1

                std::cout << "size hex: " << fat_table1_hex.size();
                std::cout << "got free clust: " << free_clust_index;

                for (int i = 0; i < fat_table1_hex.size(); i++) { //printing hex
                    //printf("First hex is %.2X\n", fat_table1_hex.at(i));
                }

                printf("First hex is %.2X, sec is %.2X\n", free_cluster_index_first, free_cluster_index_sec); //z druheho vzit prvni a druhy / PRVNI - prvni
                //std::cout << "in hex got: " << free_cluster_hex[0] << free_cluster_hex[1] << free_cluster_hex[2];

                char convert_buffer[5]; //buffer pro prevod hex na dec
                snprintf(convert_buffer, sizeof(convert_buffer), "%.2X%.2X", free_cluster_index_first, free_cluster_index_sec); //obsahuje 16 bitu
                std::cout << "buffer is: " << convert_buffer;

                char hex_free_clust[4];
                snprintf(hex_free_clust, sizeof(hex_free_clust), "%.3X", free_clust_index);
                printf("In hex got: %s", hex_free_clust);

                char byte_to_save_first[2];
                char byte_to_save_second[2];
                char first_byte;
                char second_byte;

                byte_to_save_first[0] = hex_free_clust[1]; //druhe cislo free clust v hex
                byte_to_save_first[1] = hex_free_clust[2]; //treti cislo free clust v hex

                byte_to_save_second[0] = convert_buffer[2]; //cislo mimo
                byte_to_save_second[1] = hex_free_clust[0]; //prvni cislo free clust v hex

                std::cout << "would save: " << byte_to_save_first[0] << byte_to_save_first[1] << byte_to_save_second[0] << byte_to_save_second[1];

                first_byte = conv_char_arr_to_hex(byte_to_save_first);
                second_byte = conv_char_arr_to_hex(byte_to_save_second);

                //fat_table1_hex.at(first_index_hex_tab) = first_byte;
                //fat_table1_hex.at(first_index_hex_tab + 1) = second_byte;
                std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
                for (int i = 0; i < fat_table1_dec.size() - 3000; i++) {
                    printf("Fat on index %d got %d\n", i, fat_table1_dec.at(i));
                }

                std::vector<char> test_ride;
                test_ride.push_back(first_byte);
                test_ride.push_back(second_byte);
                write_data_to_fat_fs(-31, test_ride);

                //to_save[0] = convert_buffer[2];
                //to_save[1] = convert_buffer[3];
                //to_save[2] = convert_buffer[0];

                //std::cout << "next is: " << to_save[0] << to_save[1] << to_save[2];

                //write_data_to_fat_fs(1 - 31, fat_table1_hex); //-31, fce pocita s dat clustery
                //write_data_to_fat_fs(10 - 31, fat2_table_hex); //-31, fce pocita s dat clustery
            }
            else { //lichy index
                int first_index_hex_tab = index_to_edit * 1.5; //index volneho clusteru v hex(na dvou bajtech)

                char free_cluster_index_first = fat_table1_hex.at(first_index_hex_tab);
                char free_cluster_index_sec = fat_table1_hex.at(first_index_hex_tab + 1);

                printf("First hex is %.2X, sec is %.2X\n", free_cluster_index_first, free_cluster_index_sec); //z druheho vzit prvni a druhy / PRVNI - prvni
                //std::cout << "in hex got: " << free_cluster_hex[0] << free_cluster_hex[1] << free_cluster_hex[2];

                char convert_buffer[5]; //buffer pro prevod hex na dec
                snprintf(convert_buffer, sizeof(convert_buffer), "%.2X%.2X", free_cluster_index_first, free_cluster_index_sec); //obsahuje 16 bitu
                std::cout << "buffer is: " << convert_buffer;

                char hex_free_clust[4];
                snprintf(hex_free_clust, sizeof(hex_free_clust), "%.3X", free_clust_index);
                printf("In hex got: %s", hex_free_clust);

                char byte_to_save_first[2];
                char byte_to_save_second[2];
                char first_byte;
                char second_byte;

                byte_to_save_first[0] = hex_free_clust[2]; //treti cislo free clust v hex
                byte_to_save_first[1] = convert_buffer[1]; //cislo mimo

                byte_to_save_second[0] = hex_free_clust[0]; //prvni cislo free clust v hex
                byte_to_save_second[1] = hex_free_clust[1]; //druhe cislo free clust v hex


                std::cout << "would save: " << byte_to_save_first[0] << byte_to_save_first[1] << byte_to_save_second[0] << byte_to_save_second[1];
                printf("originally got %.2X%.2X", free_cluster_index_first, free_cluster_index_sec);

                first_byte = conv_char_arr_to_hex(byte_to_save_first);
                second_byte = conv_char_arr_to_hex(byte_to_save_second);

                fat_table1_hex.at(first_index_hex_tab) = first_byte;
                fat_table1_hex.at(first_index_hex_tab + 1) = second_byte;
                std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);
                for (int i = 0; i < fat_table1_dec.size(); i++) {
                    printf("Fat on index %d got %d\n", i, fat_table1_dec.at(i));
                }
                std::cout << "got free clust: " << free_clust_index;
                std::vector<char> test_ride;
                test_ride.push_back(first_byte);
                test_ride.push_back(second_byte);
                write_data_to_fat_fs(-31, test_ride);
            }

        }
        else { //musime se pokusit alokovat novy cluster pro soubor
            int free_clust_index = retrieve_free_cluster_index(fat_table1_dec);
            std::cout << "got free clust: " << free_clust_index;
            if (free_clust_index == -1) {
                return kiv_os::NOS_Error::Not_Enough_Disk_Space; //nebyl nalezen zadny volny cluster, koncime, cely buffer nemuze byt zapsan..
            }
            //ok, mame novy cluster, priradit souboru a zapis upravene FAT
            fat_table1_dec.at(file_clust_nums.at(file_clust_nums.size() - 1)) = free_clust_index;
            fat_table1_dec.at(free_clust_index) = 4095;
            for (int i = 0; i < fat_table1_dec.size(); i++) {
                printf("Fat on index %d got %d\n", i, fat_table1_dec.at(i));
            }
        }

        clust_data_write.clear();
    }

    std::cout << "content is: " << content_clust;
    //write_data_to_fat_fs(sector_num_vect, buffer);

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
