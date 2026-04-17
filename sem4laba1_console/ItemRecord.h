#pragma once
#include <cstdint>
#include <string>
using namespace std;

struct ItemRecord {
	char deleted;					// бит удаления (1)
	int32_t pointer_to_spec_record;	// указатель на запись файла спецификаций, содержащую данные о первом компоненте данного изделия или узла(4)
	int32_t pointer_to_next_record;	// указатель на следующую запись списка изделий (4)
	string data;					// область данных, (длина определяется при создании списка и хранится в соответствующем поле первой записи файла)
};