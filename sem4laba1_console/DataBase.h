#pragma once
#include <string>
#include <cstdio>
#include "ItemsHeader.h"
#include "SpecHeader.h"

#pragma pack(push, 8)

struct DataBase {
    FILE* items_file;
    FILE* spec_file;
    ItemsHeader items_header;
    SpecHeader spec_header;
    std::string items_filename;
    std::string spec_filename;

    DataBase() : items_file(nullptr), spec_file(nullptr) {}
};

#pragma pack(pop)