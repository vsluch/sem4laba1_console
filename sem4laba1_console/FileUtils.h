#pragma once
#include <string>
#include <cstdio>
#include "DataBase.h"
#include <stdio.h>
#include <iostream>
#include "ItemsHeader.h"
#include "ItemRecord.h"
#include "SpecHeader.h"
#include "SpecRecord.h"
#include "DataBase.h"
#include <vector>
#include <algorithm>
#include <map>
#include <cstdio>

using namespace std;

void create(DataBase& data_base, string filename, uint16_t max_name_len, string spec_file_name);
void closeFiles(FILE*& items_file, FILE*& spec_file);
bool overwritingFile();
void open(DataBase& data_base, string filename);
ItemRecord readItemRecord(DataBase& db, int32_t offset);
void writeItemRecord(DataBase& db, int32_t offset, const ItemRecord& rec);
int32_t addItemRecord(DataBase& db, const ItemRecord& rec);
SpecRecord readSpecRecord(DataBase& db, int32_t offset);
void writeSpecRecord(DataBase& db, int32_t offset, const SpecRecord& rec);
int32_t addSpecRecord(DataBase& db, const SpecRecord& rec);
int32_t findItemByName(DataBase& db, const string& name);
void cmdInputComponent(DataBase& db, const string& name, const string& type);
void cmdInputSpec(DataBase& db, const string& parentName, const string& childName, uint16_t quantity);
bool hasReferences(DataBase& db, int32_t itemOffset);
void markSpecChainDeleted(DataBase& db, int32_t firstSpecOffset);
void cmdDeleteComponent(DataBase& db, const string& name);
void cmdDeleteSpec(DataBase& db, const string& parentName, const string& childName);
void rebuildItemsOrder(DataBase& db);
void cmdRestore(DataBase& db, const string& name);
void printSpecification(DataBase& db, int32_t componentOffset, int32_t specOffset, int level);
void cmdPrint(DataBase& db, const string& name);
void cmdHelp();
void cmdExit(DataBase& db);
void cmdTruncate(DataBase& db);


