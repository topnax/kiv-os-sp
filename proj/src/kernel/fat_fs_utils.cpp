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
        i += 6; //prvni cluster + velikost souboru

        directory_content.push_back(dir_item);
    }

    return directory_content;
}

/*
* Vrati cislo clusteru, na kterem zacina pozadovana slozka, pokud zadana cesta existuje. Pokud neexistuje, pak bude cluster == -1.
* start_cluster = pocatecni cluster, od ktereho zacneme prohledavat
* fat_table_dec = fat tabulka, dle ktere dojde k ziskani informaci o lokaci souboru (v dec formatu)
* path - polozky v zadane ceste
/**/
directory_item retrieve_item_cluster(int start_cluster, std::vector<int> fat_table_dec, std::vector<std::string> path) {
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
        while (dir_item_number == -1 && j < cur_folder_items.size()) { //dokud nebyla nalezena slozka s odpovidajicim nazvem
            directory_item dir_item = cur_folder_items.at(j);

            if (dir_item.filename.compare(path.at(i)) == 0 && dir_item.extension.length() == 0 && dir_item.filezise == 0) { //pokud sedi nazev a nejedna se o soubor => nalezena odpovidajici slozka v ceste
                dir_item_number = j;
                std::cout << "FOUUUUND item name!!!!" << dir_item.filename << "size: " << dir_item.filezise;
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
        std::cout << "Returned root dir size: " << directory_content.size();
        std::cout << "Root print - start\n";
        for (int i = 0; i < directory_content.size(); i++) {
            std::cout << "Content is:" << directory_content.at(i).filename << ", clust: " << directory_content.at(i).first_cluster << "\n";
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
            for (int j = 0; j < directory_content.size(); j++) { //vypis obsahu jednotlivych struktur
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
        if (dir_cont[i] == 0) { /* pokud nazev zacina 0, pak neni polozka obsazena -> ani zadna dalsi, cely obsah slozky uz je vypsan */
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
    std::vector<int> sector_list; //seznam sektorù, na kterých se data souboru nachází

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