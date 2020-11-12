#pragma once
struct directory_item {
	char* filename; //nazev souboru
	char* extension; //pripona souboru
	int filezise; //velikost souboru (byt)
	int first_cluster; //prvni cluster souboru
};