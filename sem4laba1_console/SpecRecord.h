#pragma once
#include <cstdint>
using namespace std;

#pragma pack(push, 1)
struct SpecRecord {
	char deleted;					// бит удаления (1)
	int32_t pointer_to_item_record;	// указатель на запись файла списка изделий, содержащую наименование компонента спецификации(4)
	uint16_t quantity;				// кратность вхождения (2)
	int32_t pointer_to_next_record;	// указатель на следующую запись списка-спецификации (4)
};
// 11 байт всего
#pragma pack(push)
