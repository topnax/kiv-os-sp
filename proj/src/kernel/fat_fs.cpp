//
// Created by Stanislav Král on 06.11.2020.
//

#include "fat_fs.h"
#include "fat_fs_utils.h"
#include <iostream>
#include <algorithm>

std::vector<unsigned char> first_fat_table_hex; //obsah prvni fat tabulky v hex formatu
std::vector<int> first_fat_table_dec; //obsah prvni fat tabulky v desitkove soustave, prevedene z hex

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

Fat_Fs::Fat_Fs(uint8_t disk_number, kiv_hal::TDrive_Parameters disk_parameters): disk_number(disk_number), disk_parameters(disk_parameters) {
    first_fat_table_hex = load_first_fat_table_bytes(); //nacteni fat tabulky v hex
    first_fat_table_dec = convert_fat_table_to_dec(first_fat_table_hex); //prevod na dec
}

kiv_os::NOS_Error Fat_Fs::read(File file, size_t size, size_t offset, std::vector<char> &out) {
    if (((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) != 0) || ((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0)) { //ctu slozku
        std::vector<kiv_os::TDir_Entry> folder_entries;
        std::vector<char> folder_entries_char;

        kiv_os::NOS_Error read_dir_res = readdir(file.name, folder_entries);

        if (read_dir_res == kiv_os::NOS_Error::Success) { //cteni ok, prevod na vector charu
            folder_entries_char = Fat_Fs::generate_dir_vector(folder_entries);
        }

        if ((offset + size) <= (folder_entries_char.size() * sizeof(kiv_os::TDir_Entry))) { //offset mensi nez velikost souboru, ok muzeme cist
            for (int i = 0; i < size; i++) {
                out.push_back(folder_entries_char.at(offset + i));
            }

            return kiv_os::NOS_Error::Success;
        }
        else { //mimo rozsah souboru
            return kiv_os::NOS_Error::IO_Error;
        }
    }
    else { //jdeme cist klasicky soubor
        std::vector<int> file_clust_nums = retrieve_sectors_nums_fs(first_fat_table_dec, file.handle); //ziskani seznamu clusteru, na kterych se soubor nachazi

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

            return kiv_os::NOS_Error::IO_Error;
        }
        else { //rozsah ok
            to_read = size;

            for (int i = 0; i < to_read; i++) {
                out.push_back(file_all_clust.at(offset + i));
            }

            return kiv_os::NOS_Error::Success;
        }
    }
}

kiv_os::NOS_Error Fat_Fs::readdir(const char *name, std::vector<kiv_os::TDir_Entry> &entries) {
    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste

    if (folders_in_path.size() > 0 && strcmp(folders_in_path.at(folders_in_path.size() - 1).data(), ".") == 0) {
        folders_in_path.pop_back(); //odstranit .
    }

    if (folders_in_path.size() == 0) { //chceme ziskat root obsah, pak break
        std::vector<unsigned char> root_dir_cont = read_data_from_fat_fs(19 - 31, 14); //root slozka zacina na sektoru 19 a ma 14 sektoru (fce pricita default 31, pocita s dat. sektory; proto odecteni 31)
        entries = retrieve_dir_items(14, root_dir_cont, true);

        return kiv_os::NOS_Error::Success;
    }

    //nalezeni slozky ciloveho clusteru
    directory_item dir_item = retrieve_item_clust(19, first_fat_table_dec, folders_in_path);
    int first_cluster_fol = dir_item.first_cluster; //prvni cluster slozky

    std::vector<int> folder_cluster_nums = retrieve_sectors_nums_fs(first_fat_table_dec, first_cluster_fol); //nalezeni vsech clusteru slozky

    std::vector<unsigned char> one_clust_data; //data jednoho clusteru
    std::vector<unsigned char> all_clust_data; //data veskerych clusteru, na kterych je slozka rozmistena
    for (int i = 0; i < folder_cluster_nums.size(); i++) { //projdeme nactene clustery
        one_clust_data = read_data_from_fat_fs(folder_cluster_nums[i], 1); //ziskani bajtu slozky v ramci jednoho sektoru

        for (int j = 0; j < one_clust_data.size(); j++) { //vlozeni obsahu jednoho sektoru do celkoveho seznamu danych sektoru
            all_clust_data.push_back(one_clust_data.at(j));
        }
    }

    entries = retrieve_dir_items(folder_cluster_nums.size(), all_clust_data, false);

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat_Fs::open(const char *name, kiv_os::NOpen_File flags, uint8_t attributes, File &file) {
    file = File {};
    file.name = const_cast<char*>(name);
    file.position = 0; //aktualni pozice, zaciname na 
    //file.size - zjisteno v dalsi casti fce
    //file.handle - zjisteno v dalsi casti fce
    //file.attributes - zjisteno v dalsi casti fce
   
    //najit cil a ulozit do handle souboru
    int32_t target_cluster;

    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    if (folders_in_path.size() > 0 && strcmp(folders_in_path.at(folders_in_path.size() - 1).data(), ".") == 0) {
        std::cout << "removing .\n";
        folders_in_path.pop_back(); //odstranit .
    }
    directory_item dir_item = retrieve_item_clust(19, first_fat_table_dec, folders_in_path); //pokus o otevreni existujiciho souboru)
    target_cluster = dir_item.first_cluster;
    std::cout << "Target cluster is " << target_cluster << "\n";

    if (target_cluster == -1) { //soubor / slozka nenalezena; zatim NEEXISTUJE
        if (flags == kiv_os::NOpen_File::fmOpen_Always) { //soubor / slozka musi existovat, aby byl otevren
            std::cout << "file not found in open!\n";
            return kiv_os::NOS_Error::File_Not_Found;
        }
        else { //soubor / slozka nemusi existovat, pokusim se vytvorit
            dir_item.attribute = attributes; //pouziji pridelene atributy u nove vytvorene slozky / souboru
            dir_item.filezise = 0;
        
            printf("Assigned attribute: %.2X\n", attributes);
            std::cout << "Non existent, create \n";
            bool created = false;

            if (dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID) || dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) { //vytvorit slozku
                kiv_os::NOS_Error result = mkdir(name, attributes);
                std::cout << "Creating new folder: IN OPEN\n";
                if (result == kiv_os::NOS_Error::Not_Enough_Disk_Space) { //nebyl dostatek mista na disku (chybi cluster), slozka  nebyla vytvorena
                    created = false;
                }
                else { //slozka vytvorena, dostala prideleny cluster
                    created = true;
                }
            }
            else { //pokus vytvorit soubor
                int result = create_file(name, attributes, first_fat_table_dec, first_fat_table_hex);
                std::cout << "I am working with file: " << dir_item.filezise << "\n";
                file.size = dir_item.filezise; //prideleni velikosti souboru

                if (result == -1) { //nebyl dostatek mista na disku
                    created = false;
                }
                else { //ok, soubor vytvoren
                    created = true;
                }
            }
            
            if (!created) { //koncime, pokud nenalezen volny cluster
                return kiv_os::NOS_Error::Not_Enough_Disk_Space;
            }
            else { //priradim novy cluster
                target_cluster = retrieve_item_clust(19, first_fat_table_dec, folders_in_path).first_cluster;
                std::cout << "After creation found on" << target_cluster;
            }
        }
    }
    else {
        std::cout << "Exists on cluster " << target_cluster;
    }

    //v teto fazi uz tedy soubor / slozka existuje nebo byl vytvoren, dalsi prace s objektem je v obou pripadech shodna...
    file.attributes = dir_item.attribute;
    file.handle = target_cluster; //handler bude cislo clusteru
    std::vector<int> sector_nums = retrieve_sectors_nums_fs(first_fat_table_dec, file.handle);

    if (dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID) || dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) { //jedna se o slozku, pridelit velikost vzorec: pocet_polozek_slozka * sizeof(TDir_Entry)
        std::vector<kiv_os::TDir_Entry> dir_entries_size; //pro zjisteni poctu polozek ve slozce
        readdir(file.name, dir_entries_size);

        dir_item.filezise = dir_entries_size.size() * sizeof(kiv_os::TDir_Entry);
        file.size = dir_item.filezise;
    }
    else {
        file.size = dir_item.filezise; //prideleni velikosti souboru
    }

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat_Fs::mkdir(const char *name, uint8_t attributes) {
    //rozdelit na jednotlive slozky cesty => n polozek, hledame cluster n - 1 ; posledni prvek (n) bude nazev nove vytvarene slozky
    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    std::string new_folder_name = folders_in_path.at(folders_in_path.size() - 1); //nazev nove slozky

    folders_in_path.pop_back(); //posledni polozka v seznamu je nazev nove slozky, tu ted nehledame
    int result = create_folder(name, attributes, first_fat_table_dec, first_fat_table_hex);

    if (result == 0) {
        return kiv_os::NOS_Error::Success;
    }
    else {
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    }
}

kiv_os::NOS_Error Fat_Fs::rmdir(const char *name) {
    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste
    directory_item dir_item = retrieve_item_clust(19, first_fat_table_dec, folders_in_path); //nalezt cilovou slozku

    if (dir_item.first_cluster == -1) { //polozka nenalezena
        return kiv_os::NOS_Error::File_Not_Found;
    }
    //pokracujem s pokusem o smazani

    std::vector<int> sectors_nums_data = retrieve_sectors_nums_fs(first_fat_table_dec, dir_item.first_cluster);
    std::vector<directory_item> items_folder = retrieve_folders_cur_folder(first_fat_table_dec, dir_item.first_cluster);
    if (items_folder.size() != 0) { //slozka neni prazdna, nejde smazat
        return kiv_os::NOS_Error::Directory_Not_Empty;
    }

    //slozka je prazdna, pokracujeme s premazanim jejich clusteru
    std::vector<char> format;
    for (int i = 0; i < 512; i++) { //prazdno
        format.push_back(0);
    }

    for (int i = 0; i < sectors_nums_data.size(); i++) {
        std::vector<unsigned char> modified_bytes = convert_num_to_bytes_fat(sectors_nums_data.at(i), first_fat_table_hex, 0);
        first_fat_table_hex.at(sectors_nums_data.at(i) * 1.5) = modified_bytes.at(0); //oznacit cluster jako volny v hex tabulce
        first_fat_table_hex.at((sectors_nums_data.at(i) * 1.5) + 1) = modified_bytes.at(1);
        first_fat_table_dec.at(sectors_nums_data.at(i)) = 0; //oznacit cluster jako volny v dec tabulce
        write_data_to_fat_fs(sectors_nums_data.at(i), format); //prepsat prideleny cluster
    }
    save_fat_tables(first_fat_table_hex); //ulozit fat tabulku

    //ted vymazat odkaz z nadrazene slozky - START
    std::string filename = folders_in_path.at(folders_in_path.size() - 1);
    folders_in_path.pop_back();

    bool upper_root = false;

    int start_sector = -1;
    std::vector<int> sectors_nums_data_upper; //sektory nadrazene slozky
    if (folders_in_path.size() == 0) { //nadrazena slozka je v rootu
        upper_root = true;
        start_sector = 19;

        for (int i = 19; i < 33; i++) {
            sectors_nums_data_upper.push_back(i);
        }
    }
    else { //klasicka slozka, ne root
        directory_item target_folder = retrieve_item_clust(19, first_fat_table_dec, folders_in_path);
        sectors_nums_data_upper = retrieve_sectors_nums_fs(first_fat_table_dec, target_folder.first_cluster);

        start_sector = sectors_nums_data_upper.at(0);
    }

    std::vector<directory_item> items_folder_upper = retrieve_folders_cur_folder(first_fat_table_dec, start_sector); //ziskani obsahu nadrazene slozky

    //get number of folder to delete
    int to_remove = -1;

    for (int i = 0; i < items_folder_upper.size(); i++) { //kontrola poradi v dane slozce
        std::string item_to_check = "";
        directory_item dir_item = items_folder_upper.at(i);

        if (!dir_item.extension.empty()) {
            item_to_check = dir_item.filename + "." + dir_item.extension;
        }
        else {
            item_to_check = dir_item.filename;
        }

        if (item_to_check.compare(filename) == 0) {
            to_remove = i;
            break; //nalezen index souboru, koncime
        }
    }

    std::vector<char> fol_cont; //kompletni obsah slozky, neupraveny
    for (int i = 0; i < sectors_nums_data_upper.size(); i++) { //projit clustery nadrazene slozky
        std::vector<unsigned char> one_clust_data;
        if (upper_root) {
            one_clust_data = read_data_from_fat_fs(sectors_nums_data_upper.at(i) - 31, 1);
        }
        else {
            one_clust_data = read_data_from_fat_fs(sectors_nums_data_upper.at(i), 1);
        }

        for (int j = 0; j < SECTOR_SIZE_B; j++) {
            fol_cont.push_back(one_clust_data.at(j));
        }
    }

    int index_to_remove;
    if (upper_root) {
        index_to_remove = to_remove + 1;
    }
    else {
        index_to_remove = to_remove + 2;
    }

    //z fol_cont odstranit nechteny dir_entry - jedna polozka ma 32 bajtu
    fol_cont.erase(fol_cont.begin() + ((index_to_remove) * 32), fol_cont.begin() + ((index_to_remove * 32) + 32)); 
    for (int i = 0; i < 32; i++) { //doplnit 0 zbytek
        fol_cont.push_back(0);
    }

    //zformatovat clustery - zapsat 0 - START
    for(int i = 0; i < sectors_nums_data_upper.size(); i++) {
        if (upper_root) {
            write_data_to_fat_fs(sectors_nums_data_upper.at(i) - 31, format);
        }
        else {
            write_data_to_fat_fs(sectors_nums_data_upper.at(i), format);
        }
    }
    //zformatovat clustery - zapsat 0 - KONEC

    //ted vymazat odkaz z nadrazene slozky - KONEC

    for (int i = 0; i < sectors_nums_data_upper.size(); i++) {
        std::vector<char> clust_to_save; // data jednoho clusteru

        for (int j = 0; j < SECTOR_SIZE_B; j++) {
            clust_to_save.push_back(fol_cont.at((i * SECTOR_SIZE_B) + j));
        }

        if (upper_root) {
            write_data_to_fat_fs(sectors_nums_data_upper.at(i) - 31, clust_to_save);
        }
        else {
            write_data_to_fat_fs(sectors_nums_data_upper.at(i), clust_to_save);
        }
    }
    //ted vymazat odkaz z nadrazene slozky - KONEC

    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat_Fs::write(File file, std::vector<char> buffer, size_t size, size_t offset, size_t &written) {
    if (offset > file.size) { //nemuzeme v souboru nechat prazdne misto
        return kiv_os::NOS_Error::IO_Error;
    }

    std::vector<int> file_clust_nums = retrieve_sectors_nums_fs(first_fat_table_dec, file.handle); //seznam clusteru, na kterych se soubor nachazi

    int cluster_fully_occ_bef = (offset) / SECTOR_SIZE_B; //pocet plne obsazenych clusteru pred offsetem

    int sector_num; //poradi sektoru, na ktery ma byt zapsano ; zaokrouhleno nahoru

    if (offset == 0) {
        sector_num = 1;
    }
    else {
        sector_num = offset / SECTOR_SIZE_B + (offset % SECTOR_SIZE_B != 0);
    }

    // TODO this causes trouble
    printf("zx sector_num=%d, %d\n", sector_num, file_clust_nums.size());
    int sector_num_vect = file_clust_nums.at(sector_num - 1); //nalezeni odpovidajiciho sektoru v poradi v ramci vektoru
    printf("wv\n");

    int bytes_to_save_clust = offset % SECTOR_SIZE_B; //pocet bajtu, ktere jsou na clusteru, na ktery se bude zapisovat pred offsetem (ty chceme uchovat)

    std::vector<unsigned char> data_last_clust = read_data_from_fat_fs(sector_num_vect, 1);
    std::vector<unsigned char> data_to_write;

    for (int i = 0; i < bytes_to_save_clust; i++) { //pruchod bajtu pred offsetem na danem clusteru
        data_to_write.push_back(data_last_clust.at(i));
    }

    for (int i = 0; i < buffer.size(); i++) { //pridani obsahu bufferu pro zapis
        data_to_write.push_back(buffer.at(i));
    }

    std::string content_clust;

    for (int i = 0; i < data_to_write.size(); i++) {
        content_clust.push_back(data_to_write.at(i));
    }

    int written_bytes = 0 - bytes_to_save_clust;

    //v bufferu je ulozen obsah vsech clusteru, ktere maji byt prepsany - zaciname od clusteru sector_num_vect, pripadne pak posun na dalsi, pokud buffer > 512 - teoreticky zapis na vice clusteru
    int clusters_count = data_to_write.size() / SECTOR_SIZE_B + (data_to_write.size() % SECTOR_SIZE_B != 0); //pocet clusteru, pres ktere bude buffer ulozen

    std::vector<char> clust_data_write; //data pro zapis do jednoho clusteru
    for (int i = 0; i < clusters_count; i++) {
        if (i == (clusters_count - 1)) { //posledni cluster, nemusi byt vyuzity cely
            for (int j = 0; j < data_to_write.size() - (i * SECTOR_SIZE_B); j++) {
                clust_data_write.push_back(data_to_write.at(j + (i * SECTOR_SIZE_B)));
            }
        }
        else { //ćluster, ktery neni posledni bude vyuzit cely
            for (int j = 0; j < SECTOR_SIZE_B; j++) {
                clust_data_write.push_back(data_to_write.at(j + (i * SECTOR_SIZE_B)));
            }
        }

        if (sector_num - 1 + i < file_clust_nums.size()) { //ok, vejdeme se do alokovanych clusteru
            write_data_to_fat_fs(file_clust_nums.at(sector_num - 1 + i), clust_data_write); //zapis dat do fs, postupne posun po alokovanych clusterech
            written_bytes += clust_data_write.size();
        }
        else { //musime se pokusit alokovat novy cluster pro soubor
            printf("ab\n");
            int free_clust_index = retrieve_free_cluster_index(first_fat_table_dec);

            printf("cd\n");
            if (free_clust_index == -1) {
                save_fat_tables(first_fat_table_hex); //zapis fat pred opustenim

                 //updatovat velikost souboru v nadrazene slozce
                int newly_written_bytes = (offset + written_bytes) - file.size; //na jake misto jsem se dostal - puvodni velikost souboru = pocet pridanych bajtu
                if (newly_written_bytes > 0) { //soubor byl zvetsen, update velikosti ve slozce...
                    update_size_file_in_folder(file.name, offset, file.size, newly_written_bytes, first_fat_table_dec);
                    file.size = file.size + newly_written_bytes;
                    written = written_bytes;
                }
                printf("ef\n");
                return kiv_os::NOS_Error::Not_Enough_Disk_Space; //nebyl nalezen zadny volny cluster, koncime, cely buffer nemuze byt zapsan..
            }

            //ok, mame novy cluster, prirazeni v dec tabulce
            first_fat_table_dec.at(file_clust_nums.at(file_clust_nums.size() - 1)) = free_clust_index; //posledni cluster dostane odkaz na volny
            first_fat_table_dec.at(free_clust_index) = 4095; //dosud volny cluster oznacen jako konecny

            //v hex tabulkach upravit cislo posledniho clusteru puvodne prideleno souboru - START
            int index_to_edit = file_clust_nums.at(file_clust_nums.size() - 1); //posledni cluster dostane prideleny odkaz na nove alokovany cluster - pocitano v dec
            int first_index_hex_tab = index_to_edit * 1.5; //index volneho clusteru v hex tabulce (na dvou bajtech)

            char free_cluster_index_first = first_fat_table_hex.at(first_index_hex_tab);
            char free_cluster_index_sec = first_fat_table_hex.at(first_index_hex_tab + 1);

            std::vector<unsigned char> modified_bytes = convert_num_to_bytes_fat(index_to_edit, first_fat_table_hex, free_clust_index);
            first_fat_table_hex.at(index_to_edit * 1.5) = modified_bytes.at(0);
            first_fat_table_hex.at((index_to_edit * 1.5) + 1) = modified_bytes.at(1);
            //v hex tabulkach upravit cislo posledniho clusteru puvodne prideleno souboru - KONEC

            //v hex tabulkach na indexu nove prideleneho clusteru udelit 4095 (konec souboru) - START
            first_index_hex_tab = free_clust_index * 1.5; //index zabraneho clusteru

            free_cluster_index_first = first_fat_table_hex.at(first_index_hex_tab);
            free_cluster_index_sec = first_fat_table_hex.at(first_index_hex_tab + 1);

            modified_bytes = convert_num_to_bytes_fat(free_clust_index, first_fat_table_hex, 4095);
            first_fat_table_hex.at(free_clust_index * 1.5) = modified_bytes.at(0);
            first_fat_table_hex.at((free_clust_index * 1.5) + 1) = modified_bytes.at(1);
            //v hex tabulkach na indexu nove prideleneho clusteru udelit 4095 (konec souboru) - KONEC

            write_data_to_fat_fs(free_clust_index, clust_data_write); //zapis dat na nove alokovany cluster
            written_bytes += clust_data_write.size();

            printf("gh\n");
            printf("chi\n");
        }

        clust_data_write.clear();
    }

    //po dokonceni zapisu zapsat prvni a druhou fat tabulku
    save_fat_tables(first_fat_table_hex);
    printf("jk\n");
    //updatovat velikost souboru v nadrazene slozce
    int newly_written_bytes = (offset + written_bytes) - file.size; //na jake misto jsem se dostal - puvodni velikost souboru = pocet pridanych bajtu
    if (newly_written_bytes > 0) { //soubor byl zvetsen, update velikosti ve slozce...
        update_size_file_in_folder(file.name, offset, file.size, newly_written_bytes, first_fat_table_dec);

        printf("lm\n");
        file.size = file.size + newly_written_bytes;
    }
    written = written_bytes;
    printf("fat_fs has written %d, %d\n", written, written_bytes);
    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat_Fs::unlink(const char *name) {
    return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error Fat_Fs::close(File file) {
    return kiv_os::NOS_Error::IO_Error;
}

bool Fat_Fs::file_exists(int32_t current_fd, const char* name, bool start_from_root, int32_t& found_fd)
{
    std::cout << "Calling file_exists with name: " << name << "and current fd " << current_fd << " \n";
    if (strcmp(name, ".") == 0) { //aktualni slozka
        std::cout << "Cur fol matches\n";
        found_fd = current_fd;
        std::cout << "Returning target cluster created: " << found_fd << "\n";
        return true; //aktualni slozka existuje vzdy
    }

    if (strcmp(name, "..") == 0) { //nadrazena slozka existuje, pokud nejsme v rootu
        std::cout << "Cur fol matches\n";

        std::cout << "Rading from upper: " << current_fd << "\n";
        std::vector<unsigned char> data_first_clust = read_data_from_fat_fs(current_fd, 1);
        unsigned char first_byte_addr = data_first_clust.at(58); //prvni bajt reprez. prvni cluster nadraz. slozky
        unsigned char second_byte_addr = data_first_clust.at(59); //druhy bajt reprez. prvni cluster nadraz. slozky

        std::cout << "Bytes - start\n";
        for (int i = 0; i < data_first_clust.size(); i++) {
            printf("Byte is %.2X\n", data_first_clust.at(i));
        }

        std::cout << "Bytes - end\n";

        printf("BYTE IS %.2X %.2X", first_byte_addr, second_byte_addr);

        found_fd = current_fd;
        std::cout << "Returning target cluster created: " << found_fd << "\n";
        return true; //aktualni slozka existuje vzdy
    }
    else {
        std::cout << "Not match " << name << " with: " << current_fd << "\n";
    }

    int start_cluster = -1;

    if (start_from_root) { //jdem z rootu, zacina na 19 clusteru
        start_cluster = 19;
    }
    else { //jdem z predanyho clusteru
        start_cluster = current_fd;
    }

    std::vector<std::string> folders_in_path = path_to_indiv_items(name); //rozdeleni na indiv. polozky v ceste

    directory_item dir_item = retrieve_item_clust(start_cluster, first_fat_table_dec, folders_in_path);
    found_fd = dir_item.first_cluster;

    if (dir_item.first_cluster == -1) { //polozka nenalezena
        return false;
    }
    else {
        return true;
    }
}

uint32_t Fat_Fs::get_root_fd() {
    return 19;
}
