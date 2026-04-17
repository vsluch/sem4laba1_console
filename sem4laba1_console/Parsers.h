#pragma once
#include "FileUtils.h"
using namespace std;

void inputParser(string input, DataBase& data_base);
void createParser(const string& args, string& filename, uint16_t& max_name_len, string& spec_file_name);
void openParser(const string args, string& filename);


bool tryParseUint16(const string s, uint16_t& result);
bool onlyFilenameSymbols(const string s);

