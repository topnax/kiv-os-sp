#pragma once
#include <string>

struct directory_item {
	std::string filename; //nazev souboru
	std::string extension; //pripona souboru
	int filezise; //velikost souboru (byt)
	int first_cluster; //prvni cluster souboru
};