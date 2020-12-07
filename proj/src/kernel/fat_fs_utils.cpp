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
* Prevede fat tabulku na dec, pro snazsi vyhledavani.
* fat_table_hex = obsah fat tabulky (hex)
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
    std::vector<unsigned char> file_bytes; //nactene byty souboru
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
* is_root - true, pokud byl predan obsah rootovske slozky (vynecha se prvni polozka - aktualni adresar) / jinak false (vynecha se . a ..)
/**/
std::vector<kiv_os::TDir_Entry> retrieve_dir_items(int num_sectors, std::vector<unsigned char> dir_clusters, bool is_root) {
    std::vector<kiv_os::TDir_Entry> directory_content; //obsahuje struktury TDir_Entry, ktere reprezentuji jednu entry ve slozce

    char first_clust_buff_conv[5]; //buffer pro konverzi dvou hexu na dec (reprezentuje prvni cluster souboru)
    char filesize_buff_conv[9]; //buffer pro konverzi ctyr hexu na dec (reprezentuje velikost souboru)

    for (int i = 0; i < SECTOR_SIZE_B * num_sectors;) {
        if (dir_clusters[i] == 0 || dir_clusters[i] == 246) { /* pokud nazev zacina 0, pak neni polozka obsazena -> ani zadna dalsi, cely obsah slozky uz je vypsan */
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
        i += 6; //prvni cluster + velikost souboru

        directory_content.push_back(dir_item);
    }

    if (is_root) { //vynechame prvni polozku - aktualni slozka
        directory_content.erase(directory_content.begin()); //odstraneni prvni polozky (reference na aktualni slozku)
    }
    else { //vynechame prvni dve polozky - . a ..
        directory_content.erase(directory_content.begin()); //odstraneni prvni polozky (reference na aktualni slozku)
        directory_content.erase(directory_content.begin()); //odstraneni druhe polozky (reference na nadrazenou slozku)
    }

    return directory_content;
}

/*
* Vrati cislo clusteru, na kterem zacina pozadovana slozka ci soubor, pokud zadana cesta existuje. Pokud neexistuje, pak bude cluster == -1.
* start_cluster = pocatecni cluster, od ktereho zacneme prohledavat
* fat_table_dec = fat tabulka, dle ktere dojde k ziskani informaci o lokaci souboru (v dec formatu)
* is_folder = true, pokud je cilem slozka ; v pripade souboru false
* path = polozky v zadane ceste
/**/
directory_item retrieve_item_clust(int start_cluster, std::vector<int> fat_table_dec, bool is_folder, std::vector<std::string> path) {
    std::cout << "retrieve_item_cluster with num!!!!" << start_cluster;

    int traversed_sector_folder = start_cluster; //cislo sektoru, na kterem zacina aktualne prochazena slozka - zaciname v rootu
    int dir_item_number = -1; //poradi hledane slozky v ramci prave prochazene slozky
    std::vector<directory_item> cur_folder_items; //obsahuje polozky nachazejici se v prave prochazene slozce (slozek bude vice, pokud relativni cesta)

    for (int i = 0; i < path.size(); i++) { //pruchod seznamem slozek v ceste (muze jich byt vice)
        dir_item_number = -1; //poradi slozky do ktere se mam presunout; v ramci vectoru cur_folder_items
        cur_folder_items = retrieve_folders_cur_folder(fat_table_dec, traversed_sector_folder); //nactu obsah aktualni slozky
        for (int i = 0; i < cur_folder_items.size(); i++) {
            //std::cout << "Got name: " << cur_folder_items.at(i).filename << "." << cur_folder_items.at(i).filename;
        }

        int j = 0;

        std::string item_to_check = "";  //nazev souboru + pripona
        while (dir_item_number == -1 && j < cur_folder_items.size()) { //dokud nebyla nalezena slozka s odpovidajicim nazvem
            directory_item dir_item = cur_folder_items.at(j);

            // TODO remove/rework this, this should take "dir ." command in root folder into consideration
            if (start_cluster == 19 && path.at(i) == ".") {
                auto di = directory_item{
                    ".",
                    "",
                    static_cast<int>(cur_folder_items.size() * sizeof(kiv_os::TDir_Entry)),
                    19
                };
                return di;
            }

            if (!dir_item.extension.empty()) {
                item_to_check = dir_item.filename + "." + dir_item.extension;
            }
            else {
                item_to_check = dir_item.filename;
            }

            // TODO replaced this
            //if (i == (path.size() - 1) && !is_folder) { //prochazi se posledni polozka v ceste a hleda se cluster souboru
            //    if (path.at(i).compare(item_to_check) == 0 && dir_item.filezise != 0) { //pokud sedi nazev a JEDNA se o soubor => nalezen cilovy soubor v ceste
            //        dir_item_number = j;
            //        std::cout << "FOUUUUND item name!!!!" << dir_item.filename << "size: " << dir_item.filezise << "\n";
            //    }
            //}
            //else { //hledame cluster slozky
            //    if (path.at(i).compare(item_to_check) == 0 && dir_item.extension.length() == 0 && dir_item.filezise == 0) { //pokud sedi nazev a nejedna se o soubor => nalezena odpovidajici slozka v ceste
            //        dir_item_number = j;
            //        std::cout << "FOUUUUND item name!!!!" << dir_item.filename << "size: " << dir_item.filezise << "\n";
            //    }
            //}
            // TODO with this, because is_folder flag is nonexistent
            if (path.at(i).compare(item_to_check) == 0 && dir_item.filezise != 0) { //pokud sedi nazev a JEDNA se o soubor => nalezen cilovy soubor v ceste
                dir_item_number = j;
                std::cout << "FOUUUUND item name!!!!" << dir_item.filename << "size: " << dir_item.filezise << "\n";
            } else if (path.at(i).compare(item_to_check) == 0 && dir_item.extension.length() == 0 && dir_item.filezise == 0) { //pokud sedi nazev a nejedna se o soubor => nalezena odpovidajici slozka v ceste
                dir_item_number = j;
                std::cout << "FOUUUUND item name!!!!" << dir_item.filename << "size: " << dir_item.filezise << "\n";
            }

            j++;
        }

        if (dir_item_number != -1) { //slozka v ceste, pokracovat hledanim dalsi slozky
            directory_item dir_item = cur_folder_items.at(dir_item_number);
            traversed_sector_folder = dir_item.first_cluster;

            if (dir_item.first_cluster == 0) { //pokud se jedna o root slozku, vyjimka
                dir_item.first_cluster = 19;
            }
        }
        else {
            break;
        }
    }

    directory_item dir_item;
    if (dir_item_number != -1) { //pozadovana cesta nalezena, ok
        dir_item = cur_folder_items.at(dir_item_number);

        if (dir_item.first_cluster == 0) { //pokud se jedna o root slozku, vyjimka
            dir_item.first_cluster = 19;
        }
    }
    else { //nektera ze slozek v ceste nebyla nalezena, err
        dir_item.first_cluster = -1;
        dir_item.filezise = -1;
    }

    return dir_item;
}

/*
* Vrati obsah aktualni slozky (podslozky i soubory) reprezentovane strukturami directory_item.
* fat_table_dec = fat tabulka, dle ktere dojde k ziskani informaci o lokaci souboru (v dec formatu)
* working_dir_sector - cislo sektoru, na kterem zacina prave prochazena slozka
/**/
std::vector<directory_item> retrieve_folders_cur_folder(std::vector<int> fat_table_dec, int working_dir_sector) {
    if (working_dir_sector == 19) { //jsme v root slozce, fix velikost -> nehledat ve FAT tabulce, nema tam udaje!
        std::vector<unsigned char> root_dir_cont = read_data_from_fat_fs(19 - 31, 14); //ziskani bajtu slozky v po sobe jdoucich clusterech - zacina na 19 sektoru (fce pricita 31, pocita s dat. sektory, proto odecteme 31);        
        std::vector<directory_item> directory_content = get_dir_items(14, root_dir_cont); //ziskani obsahu slozky (struktury directory_item, ktere reprezentuji jednu entry ve slozce)
        
        directory_content.erase(directory_content.begin()); //odstraneni prvni polozky (reference na aktualni slozku)
        std::cout << "Returned root dir size: " << directory_content.size();
        std::cout << "Root print - start\n";
        for (int i = 0; i < directory_content.size(); i++) {
            std::cout << "Content is:" << directory_content.at(i).filename << ", clust: " << directory_content.at(i).first_cluster << ", size: " << directory_content.at(i).filezise << "\n";
        }
        std::cout << "Root print - end\n";

        std::cout << "In root folder got size: " << directory_content.size();
        return directory_content;
    }
    else { //slozka neni rootovska, velikost neni fixni, nevime clustery - data nacist z FAT tabulky
        std::vector<int> sectors_nums_data = retrieve_sectors_nums_fs(fat_table_dec, working_dir_sector); //zjistime sektory, na kterych se slozka nachazi
        std::cout << "To search on number of clusters: " << sectors_nums_data.size();
        for (int i = 0; i < sectors_nums_data.size(); i++) {
            std::cout << "Read from clust: " << sectors_nums_data.at(i);
        }

        std::vector<unsigned char> retrieved_data_clust; //obsahuje data jednoho clusteru
        std::vector<directory_item> all_dir_items; //obsahuje veskere podslozky i soubory ve slozce (pres vsechny clustery...)

        for (int i = 0; i < sectors_nums_data.size(); i++) { //projdeme nactene clustery
            retrieved_data_clust = read_data_from_fat_fs(sectors_nums_data[i], 1); //ziskani bajtu slozky v ramci jednoho sektoru
            std::vector<directory_item> directory_content = get_dir_items(1, retrieved_data_clust); //prevod na struktury, kazda reprez. jednu slozku
            std::cout << "Got size before printing: " << directory_content.size() << ", from cluster: " << sectors_nums_data.at(i) << "\n";

            int j = 0;
            if (i == 0) { //prvni cluster u slozky, co ma vice clusteru - preskocit . a ..
                j = 2;
            }
            else {
                j = 0;
            }

            for (; j < directory_content.size(); j++) { //vypis obsahu jednotlivych struktur
                directory_item dir_item = directory_content.at(j);
                std::cout << "Content is:" << dir_item.filename << ", clust: " << dir_item.first_cluster << "\n";

                all_dir_items.push_back(dir_item);
            }
        }

        std::cout << "Other folder print - start\n";
        for (int i = 0; i < all_dir_items.size(); i++) {
            //std::cout << "Content is:" << all_dir_items.at(i).filename << ", clust: " << all_dir_items.at(i).first_cluster << "\n";
        }
        std::cout << "Other folder print - end\n";
        std::vector<int> sector_nums = retrieve_sectors_nums_fs(fat_table_dec, 2192);
        std::cout << "Got sizeee: " << sector_nums.size();
        for (int i = 0; i < sector_nums.size(); i++) {
            std::cout << sector_nums[i] << "\n";
        }
        std::cout << "In other folder got size: " << all_dir_items.size();

        return all_dir_items;
    }
}

/*
* Vytvori vector obsahujici objekty directory_item reprezentujici obsah slozky.
* num_sectors = pocet sektoru vyhrazenych pro obsah dane slozky (root slozka ma 14 sektoru)
* dir_cont = vector obsahujici byty reprezentujici jednu slozku
/**/
std::vector<directory_item> get_dir_items(int num_sectors, std::vector<unsigned char> dir_cont) {
    std::vector<directory_item> directory_content; //obsahuje struktury directory_item, ktere reprezentuji jednu entry ve slozce

    char first_clust_buff_conv[5]; //buffer pro konverzi dvou hexu na dec (reprezentuje prvni cluster souboru)
    char filesize_buff_conv[9]; //buffer pro konverzi ctyr hexu na dec (reprezentuje velikost souboru)

    for (int i = 0; i < SECTOR_SIZE_B * num_sectors;) {
        if (dir_cont[i] == 0 || dir_cont[i] == 246) { /* pokud nazev zacina 0, pak neni polozka obsazena -> ani zadna dalsi, cely obsah slozky uz je vypsan */
            break;
        }

        directory_item dir_item; //nova entry

        dir_item.filename = "";

        int j = 0;
        bool end_encountered = false; //true pokud narazime na konec nazvu (fatka ma fix na 8 byt)
        while (j < DIR_FILENAME_SIZE_B && !end_encountered) { //cteni nazvu souboru
            if ((int)dir_cont[i] == 32) { //konec nazvu souboru (fix na 8 byt)
                end_encountered = true;
                i += DIR_FILENAME_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 8 byt)
            }
            else {
                dir_item.filename.push_back(dir_cont[i++]);
                j++;
            }
        }

        j = 0;
        end_encountered = false;
        dir_item.extension = "";
        while (j < DIR_EXTENSION_SIZE_B && !end_encountered) { //cteni pripony
            if ((int)dir_cont[i] == 32) { //konec pripony souboru (fix na 3 byt)
                end_encountered = true;
                i += DIR_EXTENSION_SIZE_B - j; //preskoceni nevyuzitych vyhrazenych clusteru (dopocet do 3 byt)
            }
            else {
                dir_item.extension.push_back(dir_cont[i++]);
                j++;
            }
        }

        i += 15; //preskocit 15 bytu, nepotrebne info (datum vytvoreni atp.)

        snprintf(first_clust_buff_conv, sizeof(first_clust_buff_conv), "%.2X%.2X", dir_cont[i++], dir_cont[i++]);
        dir_item.first_cluster = (int)strtol(first_clust_buff_conv, NULL, 16);
        snprintf(filesize_buff_conv, sizeof(filesize_buff_conv), "%.2X%.2X%.2X%.2X", dir_cont[i++], dir_cont[i++], dir_cont[i++], dir_cont[i++]);
        dir_item.filezise = (int)strtol(filesize_buff_conv, NULL, 16);

        directory_content.push_back(dir_item);
    }

    return directory_content;
}

/*
* Vrati seznam sektoru na floppyne, ktere obsazuje specifikovany soubor.
* fat_table_dec = fat tabulka, dle ktere dojde k ziskani informaci o lokaci souboru (v dec formatu)
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
* Rozdeli cestu na jeji jednotlive slozky, ktere ulozi do vektoru textovych retezcu.
* path_file = cesta k souboru (pr. "\\FDSETUP\\SETUP\\PACKAGES\\WELCOME.ZIP")
/**/
std::vector<std::string> path_to_indiv_items(const char* path_file) {
    std::vector<std::string> folders_in_path; //vector obsahuje veskere slozky v absolutni ceste

    std::vector<char> one_item; //jedna slozka / polozka v absolutni ceste
    char character = 'A'; //jeden znak v ceste
    int counter = 0;

    while (true) { //dokud nedojdeme na konec stringu
        character = path_file[counter];

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
   
    if (folders_in_path.at(0).length() == 0) { //odstraneni polozky - byla zadana prazdna cesta... 
        folders_in_path.erase(folders_in_path.begin());
    }
    return folders_in_path;
}

/*
* Zjisti pocet volnych bytu v ramci zvoleneho clusteru.
* sector_num - cislo sektoru, u ktereho chci zjistit pocet volnych bytu
/**/
int retrieve_free_byte_count(int sector_num) {
    std::vector<unsigned char> retrieved_data_clust = read_data_from_fat_fs(sector_num, 1); //obsahuje data zkoumaneho clusteru

    int free_byte_count = 0; //pocet volnych bytu v sektoru
    int occ_byte_count = 0; //pocet obsazenych bytu v sektoru
    char convert_buffer[3]; //buffer pro prevod

    for (int i = 0; i < retrieved_data_clust.size(); i += 32) { //projdu byty a hledam volne
        snprintf(convert_buffer, sizeof(convert_buffer), "%.2X", retrieved_data_clust[i]);

        printf("got char num %i: %c%c so %i\n", i, convert_buffer[0], convert_buffer[1], retrieved_data_clust[i]);
        if (retrieved_data_clust[i] == 246 || retrieved_data_clust[i] == 0) { //v hexu F6, rezervovane misto
            free_byte_count += 32;
        }
        else {
            occ_byte_count += 32;
        }
    }

    std::cout << "got free bytes: " << free_byte_count << "and occupied: " << occ_byte_count;

    return free_byte_count;
}

/*
* Nacte ze souboroveho systemu spicifikovany pocet bytu, ktere zacinaji na danem sektoru.
* start_sector_num = sektor, od ktereho zacne zapis
* buffer_to_write = bajty, ktere budou zapsany na dane misto
/**/
void write_data_to_fat_fs(int start_sector_num, std::vector<char> buffer_to_write) {
    kiv_hal::TRegisters reg_to_write;
    kiv_hal::TDisk_Address_Packet ap_to_write;

    ap_to_write.count = buffer_to_write.size() / SECTOR_SIZE_B + (buffer_to_write.size() % SECTOR_SIZE_B != 0); //pocet sektoru, ktery ma byt zapsan ; zaokrouhleno nahoru
    ap_to_write.lba_index = start_sector_num + 31; //sektor, od ktereho zapisujeme (+31 presun na dat sektory)

    reg_to_write.rdx.l = 129; //cislo disku
    reg_to_write.rax.h = static_cast<decltype(reg_to_write.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
    reg_to_write.rdi.r = reinterpret_cast<decltype(reg_to_write.rdi.r)>(&ap_to_write);

    //posledni cluster bude cely prepsan za normalnich okolnosti, chceme cast zachovat => vytahnout data z clusteru a prepsat jen relevantni cast
    int last_sector_alloc = (start_sector_num + ap_to_write.count) - 1;
    std::vector<unsigned char> last_sector_data = read_data_from_fat_fs(last_sector_alloc, 1);
    int size_to_hold = ap_to_write.count * SECTOR_SIZE_B;

    int last_cluster_occupied = buffer_to_write.size() % SECTOR_SIZE_B;
    int start_pos = buffer_to_write.size();

    int added_bytes = 0;
    for (int i = start_pos; i < size_to_hold; i++) {
        buffer_to_write.push_back(last_sector_data.at(last_cluster_occupied + added_bytes));
        added_bytes++;
    }

    ap_to_write.sectors = static_cast<void*>(buffer_to_write.data()); //vector with bytes, that should be written to file
    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, reg_to_write);

    std::cout << "Writed!!\n" << ap_to_write.count;
}

/*
* Najde ve fat tabulce index prvniho volneho clusteru a vrati jeho index.
* fat_table_hex = obsah fat tabulky (byty)
/**/
int retrieve_free_cluster_index(std::vector<int> fat_table_dec) {
    int free_clust_num = -1;

    for (int i = 0; i < fat_table_dec.size(); i++) {
        if (fat_table_dec.at(i) == 0) { //nasel se neobsazeny cluster, koncime
            free_clust_num = i;
            break;
        }
    }

    return free_clust_num;
}

/*
* Prevod dvou pismen reprezentujicich bajt na hodnotu bajtu.
/**/
unsigned char conv_char_arr_to_hex(char char_arr[2])
{
    char buff[1];
    buff[0] = strtol(char_arr, NULL, 16); 

    return buff[0];
}

/*
* Vlozeni cisla na odpovidajici pozici pro ulozeni do fat tabulky (12 bitu reprezentuje jedno cislo, nutno vzdy upravit dva bajty).
* target_index = index, na ktery ma byt cislo ulozeno (vzhledem k tabulce v dec)
* fat_table_hex = obsah fat tabulky, hexadecimalni forma
* num_to_inject = cislo, ktere ma byt vlozeno na target_index
/**/
std::vector<unsigned char> convert_num_to_bytes_fat(int target_index, std::vector<unsigned char> fat_table_hex, int num_to_inject) {
    std::vector<unsigned char> converted_bytes;

    int first_index_hex_tab = target_index * 1.5; //index volneho clusteru v hex(na dvou bajtech)

    unsigned char free_cluster_index_first = fat_table_hex.at(first_index_hex_tab);
    unsigned char free_cluster_index_sec = fat_table_hex.at(first_index_hex_tab + 1);

    char convert_buffer[5]; //buffer pro prevod hex na dec
    snprintf(convert_buffer, sizeof(convert_buffer), "%.2X%.2X", free_cluster_index_first, free_cluster_index_sec); //obsahuje 16 bitu

    char hex_free_clust[4];
    snprintf(hex_free_clust, sizeof(hex_free_clust), "%.3X", num_to_inject);

    char byte_to_save_first[3];
    char byte_to_save_second[3];
    unsigned char first_byte;
    unsigned char second_byte;

    if (target_index % 2 == 0) { //sudy index pro zapis
        byte_to_save_first[0] = hex_free_clust[1]; //druhe cislo free clust v hex
        byte_to_save_first[1] = hex_free_clust[2]; //treti cislo free clust v hex

        byte_to_save_second[0] = convert_buffer[2]; //cislo mimo
        byte_to_save_second[1] = hex_free_clust[0]; //prvni cislo free clust v hex
    }
    else { //lichy index pro zapis
        byte_to_save_first[0] = hex_free_clust[2]; //treti cislo free clust v hex
        byte_to_save_first[1] = convert_buffer[1]; //cislo mimo

        byte_to_save_second[0] = hex_free_clust[0]; //prvni cislo free clust v hex
        byte_to_save_second[1] = hex_free_clust[1]; //druhe cislo free clust v hex
    }

    byte_to_save_first[2] = '\0';
    byte_to_save_second[2] = '\0';

    first_byte = conv_char_arr_to_hex(byte_to_save_first);
    second_byte = conv_char_arr_to_hex(byte_to_save_second);

    converted_bytes.push_back(first_byte);
    converted_bytes.push_back(second_byte);

    return converted_bytes;
}

/*
* Ulozi predanou fat tabulku.
* fat_table - fat tabulka - hex
/**/
void save_fat_tables(std::vector<unsigned char> fat_table) {
    std::vector<char> fat_table_to_save;
    for (int i = 0; i < fat_table.size(); i++) {
        fat_table_to_save.push_back(fat_table.at(i));
    }

    write_data_to_fat_fs(1 - 31, fat_table_to_save); //prvni fat
    write_data_to_fat_fs(10 - 31, fat_table_to_save); //druha fat
}

/*
* Prevod cisla v dec na 4 bajty, hex soustava.
* num_dec - cislo v desitkove soustave
/**/
std::vector<unsigned char> convert_dec_num_to_hex(long num_dec) {
    unsigned char bytes[4];
    unsigned long num = num_dec;

    bytes[0] = (num >> 24) & 0xFF;
    bytes[1] = (num >> 16) & 0xFF;
    bytes[2] = (num >> 8) & 0xFF;
    bytes[3] = num & 0xFF;

    //vracime obracene, pro ulozeni do souboru
    std::vector<unsigned char> to_return;
    to_return.push_back(bytes[3]);
    to_return.push_back(bytes[2]);
    to_return.push_back(bytes[1]);
    to_return.push_back(bytes[0]);

    return to_return;
}

/*
* Aktualizuje velikost souboru ve slozce.
/**/
void update_size_file_in_folder(char *filename_path, int offset, int original_size, int newly_written_bytes, std::vector<int> fat_table_dec) {
    std::vector<std::string> folders_in_path = path_to_indiv_items(filename_path); //rozdeleni na indiv. polozky v ceste
    std::string filename = folders_in_path.at(folders_in_path.size() - 1); //nazev souboru

    folders_in_path.pop_back(); //posledni polozka v seznamu je nazev souboru, ten ted nehledame

    int start_sector = -1;
    std::vector<int> sectors_nums_data; //sektory nadrazene slozky
    if (folders_in_path.size() == 0) { //jsme v rootu
        start_sector = 19;

        for (int i = 19; i < 33; i++) {
            sectors_nums_data.push_back(i);
        }
    }
    else { //klasicka slozka, ne root
        directory_item target_folder = retrieve_item_clust(19, fat_table_dec, true, folders_in_path); //nalezeni clusteru nadrazene slozky
        sectors_nums_data = retrieve_sectors_nums_fs(fat_table_dec, target_folder.first_cluster); //zjisteni sektoru nadrazene slozky

        start_sector = sectors_nums_data.at(0);
    }

    std::vector<directory_item> items_folder = retrieve_folders_cur_folder(fat_table_dec, start_sector);  //ziskani obsahu nadrazene slozky

    int target_index = -1; //poradi slozky

    for (int i = 0; i < items_folder.size(); i++) { //kontrola poradi souboru v dane slozce
        std::string item_to_check = "";  //nazev souboru + pripona
        directory_item dir_item = items_folder.at(i);

        if (!dir_item.extension.empty()) {
            item_to_check = dir_item.filename + "." + dir_item.extension;
        }
        else {
            item_to_check = dir_item.filename;
        }

        if (item_to_check.compare(filename) == 0) { //shoduje se nazev vcetne pripony
            target_index = i;
            break; //nalezen index soubru, koncime
        }
    }

    if (folders_in_path.size() == 0) { //pokud root, pak index +1 (neobsahuje nadrazenou slozku)
        target_index += 1;
    }
    else { //mimo root +2 (. a ..)
        target_index += 2;
    }

    std::vector<unsigned char> new_file_size_hex = convert_dec_num_to_hex(original_size + newly_written_bytes); //nova velikost = aktualni + kolik zapsano

    //zjisteni clusteru, na kterem polozka lezi
    int cluster_num = (target_index) / 16; //poradi slozky / 16 (jeden cluster mi pojme 16 polozek)
    int item_num_clust_rel = (target_index) % 16; //poradi polozky v ramci jednoho clusteru

    std::vector<unsigned char> data_clust_fol; //obsahuje data daneho clusteru se slozkou
    if (folders_in_path.size() == 0) { //jsme v root slozce
        data_clust_fol = read_data_from_fat_fs(sectors_nums_data.at(cluster_num) - 31, 1); //-31; fce cte z dat sektoru

        data_clust_fol.at(item_num_clust_rel * 32 + 28) = new_file_size_hex.at(0);
        data_clust_fol.at(item_num_clust_rel * 32 + 29) = new_file_size_hex.at(1);
        data_clust_fol.at(item_num_clust_rel * 32 + 30) = new_file_size_hex.at(2);
        data_clust_fol.at(item_num_clust_rel * 32 + 31) = new_file_size_hex.at(3);

        std::vector<char> data_to_save;
        for (int i = 0; i < data_clust_fol.size(); i++) {
            data_to_save.push_back(data_clust_fol.at(i));
        }

        write_data_to_fat_fs(sectors_nums_data.at(cluster_num) - 31, data_to_save);
    }
    else {
        data_clust_fol = read_data_from_fat_fs(sectors_nums_data.at(cluster_num), 1); //fce cte z dat sektoru

        data_clust_fol.at(item_num_clust_rel * 32 + 28) = new_file_size_hex.at(0);
        data_clust_fol.at(item_num_clust_rel * 32 + 29) = new_file_size_hex.at(1);
        data_clust_fol.at(item_num_clust_rel * 32 + 30) = new_file_size_hex.at(2);
        data_clust_fol.at(item_num_clust_rel * 32 + 31) = new_file_size_hex.at(3);

        std::vector<char> data_to_save;
        for (int i = 0; i < data_clust_fol.size(); i++) {
            data_to_save.push_back(data_clust_fol.at(i));
        }

        write_data_to_fat_fs(sectors_nums_data.at(cluster_num), data_to_save);
    }
}
/*
* Vytvori ve fs novou slozku
* vrati 0, pokud vse ok; -1 pokud uz neni misto
/**/
int create_folder(const char* folder_path) {
    std::cout << "wanna write!!\n";

    //FAT tabulka nacteni
    std::vector<unsigned char> fat_table1_hex = load_first_fat_table_bytes();
    std::vector<int> fat_table1_dec = convert_fat_table_to_dec(fat_table1_hex);

    std::vector<std::string> folders_in_path = path_to_indiv_items(folder_path);
    std::string new_fol_name = folders_in_path.at(folders_in_path.size() - 1); //nazev nove slozky

    folders_in_path.pop_back(); //posledni polozka v seznamu je nazev nove polozky
    
    //ziskani clusteru, obsahu nadrazene slozky - START
    int start_sector = -1;
    std::vector<int> sectors_nums_data; //sektory nadrazene slozky
    if (folders_in_path.size() == 0) { //jsme v rootu
        start_sector = 19;

        for (int i = 19; i < 33; i++) {
            sectors_nums_data.push_back(i);
        }
    }
    else { //klasicka slozka, ne root
        directory_item target_folder = retrieve_item_clust(19, fat_table1_dec, true, folders_in_path);
        sectors_nums_data = retrieve_sectors_nums_fs(fat_table1_dec, target_folder.first_cluster);

        start_sector = sectors_nums_data.at(0);
    }

    std::cout << "Got number cluster: " << sectors_nums_data.size() << "!!!!!!!!!!!\n\n\n";

    std::vector<directory_item> items_folder = retrieve_folders_cur_folder(fat_table1_dec, start_sector);  //ziskani obsahu nadrazene slozky
    //ziskani clusteru, obsahu nadrazene slozky - KONEC

    //nalezeni volnych clusteru pro novou slozku - potrebuji 1
    int free_index = retrieve_free_cluster_index(fat_table1_dec);
    if (free_index == -1) { //volny index uz nelze najit
        std::cout << "No clust found!!\n";
        return -1; //nebyl nalezen zadny volny cluster, koncime
    }
    else { //uprava fat tabulek a oznaceni clusteru jako zabraneho
        std::vector<unsigned char> modified_bytes = convert_num_to_bytes_fat(free_index, fat_table1_hex, 4095);
        fat_table1_hex.at(free_index * 1.5) = modified_bytes.at(0); //oznacit cluster jako konecny v hex tabulce
        fat_table1_hex.at((free_index * 1.5) + 1) = modified_bytes.at(1);
        fat_table1_dec.at(free_index) = 4095; //oznacit cluster jako konecny v dec tabulce

        std::cout << "wanna assign clust " << free_index << "\n";

        save_fat_tables(fat_table1_hex);
    }

    //priprava bufferu s datovym obsahem jedne slozky - START
    std::vector<unsigned char> to_write_subfolder;
    std::vector<char> to_save;

    int i = 0;
    for (; i < new_fol_name.length(); i++) {
        to_write_subfolder.push_back(new_fol_name.at(i));
    }

    for (; i < 8; i++) { //doplnit na 8 bytu, ve fat nazev vzdy 8 byt
        to_write_subfolder.push_back(32);
    }

    for (int i = 0; i < 3; i++) { //doplnit na 3 byty, pripona neexistuje
        to_write_subfolder.push_back(32);
    }

    //doplnit atribut - subdirectory 0x10
    to_write_subfolder.push_back(16);

    for (int i = 0; i < 14; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(32);
    }

    //na dva bajty prvni cluster
    std::vector<unsigned char> first_assigned_clust = convert_dec_to_hex_start_clus(free_index);
    for (int i = 0; i < 2; i++) {
        to_write_subfolder.push_back(first_assigned_clust.at(i));
    }

    //na 4 bajty 0 (velikost souboru, u slozky nulova)
    for (int i = 0; i < 4; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(0);
    }
    //priprava bufferu s datovym obsahem jedne slozky - KONEC

    if (folders_in_path.size() == 0) { //pokud chceme vytvorit slozku v rootu - tam se jich vejde 224
        if ((items_folder.size() + 1 + 1) <= (sectors_nums_data.size() * 16)) { //+1 pro odkaz na aktualni slozku +1 pro novou ; vejdeme se do clusteru
            //zjisteni clusteru, na kterem bude polozka lezet
            int cluster_num = (items_folder.size() + 1) / 16; //poradi clusteru / 16 (jeden cluster mi pojme 16 polozek)
            int item_num_clust_rel = (items_folder.size() + 1) % 16; //poradi polozky v ramci jednoho clusteru

            std::cout << "Total items: " << items_folder.size() << "Got cluster num: " << cluster_num << "got clust_rel: " << item_num_clust_rel;

            std::vector<unsigned char> data_clust = read_data_from_fat_fs(sectors_nums_data.at(cluster_num) - 31, 1); //-31; fce cte z dat sektoru
            for (int i = 0; i < data_clust.size(); i++) {
                printf("%c", data_clust.at(i));
            }

            for (int i = 0; i < to_write_subfolder.size(); i++) {
                data_clust.at((item_num_clust_rel * 32) + i) = to_write_subfolder.at(i);
            }

            for (int i = 0; i < data_clust.size(); i++) {
                to_save.push_back(data_clust.at(i));
            }

            write_data_to_fat_fs(sectors_nums_data.at(cluster_num) - 31, to_save);
        }
        else { //err, slozka uz se nevejde
            return -1;
        }
    }
    else { //slozka mimo root, na jeden sektor se vejde 16 polozek
        if ((items_folder.size() + 1 + 2) <= (sectors_nums_data.size() * 16)) {
            int cluster_num = (items_folder.size() + 2) / 16; //poradi slozky / 16 (jeden cluster mi pojme 16 polozek)
            int item_num_clust_rel = (items_folder.size() + 2) % 16; //poradi polozky v ramci jednoho clusteru

            std::vector<unsigned char> data_clust = read_data_from_fat_fs(sectors_nums_data.at(cluster_num), 1);
            for (int i = 0; i < data_clust.size(); i++) {
                printf("%c", data_clust.at(i));
            }

            for (int i = 0; i < to_write_subfolder.size(); i++) {
                data_clust.at((item_num_clust_rel * 32) + i) = to_write_subfolder.at(i);
            }

            for (int i = 0; i < data_clust.size(); i++) {
                to_save.push_back(data_clust.at(i));
            }

            write_data_to_fat_fs(sectors_nums_data.at(cluster_num), to_save);
        }
        else { //err, slozka uz se nevejde
            std::cout << "Neevejdem set!!\n";
            return -1;
        }
    }

    write_folder_basics_cluster(free_index, start_sector);
    return 0;
}

/*
* Zapise na dany index zakladni strukturu - . a .. (aktualni a nadrazena slozka)
/**/
void write_folder_basics_cluster(int clust_to_write_index, int upper_fol_index) {
    //vytvoreni . - START
    std::vector<char> to_write_subfolder;

    to_write_subfolder.push_back(46);
    for (int i = 0; i < 10; i++) {
        to_write_subfolder.push_back(32);
    }

    to_write_subfolder.push_back(10); //atributy

    for (int i = 0; i < 14; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(32);
    }

    //dva bajty na aktualni cluster
    std::vector<unsigned char> first_assigned_clust = convert_dec_to_hex_start_clus(clust_to_write_index);
    for (int i = 0; i < 2; i++) {
        to_write_subfolder.push_back(first_assigned_clust.at(i));
    }

    //na 4 bajty 0 (velikost souboru, u slozky nulova)
    for (int i = 0; i < 4; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(0);
    }
    //vytvoreni . - KONEC

    //vytvoreni .. - START
    to_write_subfolder.push_back(46);
    to_write_subfolder.push_back(46);

    for (int i = 0; i < 9; i++) {
        to_write_subfolder.push_back(32);
    }

    to_write_subfolder.push_back(10); //atributy

    for (int i = 0; i < 14; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(32);
    }

    //dva bajty na nadrazeny cluster
    if (upper_fol_index == 19) { //root sektor je ve fatce zapisovan jako 0
        upper_fol_index = 0;
    }
    std::vector<unsigned char> upper_clust = convert_dec_to_hex_start_clus(upper_fol_index);
    for (int i = 0; i < 2; i++) {
        to_write_subfolder.push_back(upper_clust.at(i));
    }

    //na 4 bajty 0 (velikost souboru, u slozky nulova)
    for (int i = 0; i < 4; i++) { //14 bajtu nepotrebnych - datum vytvoreni, cas modifikace...
        to_write_subfolder.push_back(0);
    }
    //vytvoreni .. - KONEC

    write_data_to_fat_fs(clust_to_write_index, to_write_subfolder);
}

/*
* Prevede cislo reprezentujici pocatecni cluster na dva bajty (pro ulozeni).
/**/
std::vector<unsigned char> convert_dec_to_hex_start_clus(int start_clust) {
    std::vector<unsigned char> converted;

    char convert_buffer[5]; //buffer pro prevod hex na dec
    snprintf(convert_buffer, sizeof(convert_buffer), "%04X", start_clust); //obsahuje 16 bitu

    char byte_to_save_first[3];
    char byte_to_save_second[3];
    unsigned char first_byte;
    unsigned char second_byte;

    byte_to_save_first[0] = convert_buffer[0];
    byte_to_save_first[1] = convert_buffer[1];
    byte_to_save_first[2] = '\0';

    byte_to_save_second[0] = convert_buffer[2];
    byte_to_save_second[1] = convert_buffer[3];
    byte_to_save_second[2] = '\0';

    first_byte = conv_char_arr_to_hex(byte_to_save_first);
    second_byte = conv_char_arr_to_hex(byte_to_save_second);

    converted.push_back(second_byte); //pri ukladani bajty prohodit
    converted.push_back(first_byte);

    return converted;
}