#pragma once
#include <cstdint>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#pragma pack(push, 1)	// отключение выравнивания
struct ItemsHeader {
	char signature[2];					// сигратура (PS)
	uint16_t data_lenght;				// длина записи данных (2)
	int32_t pointer_to_first_record;	// указатель на логически первую запись списка изделий (4)
	int32_t pointer_to_free_memory;		// указатель на свободную область файла
	char spec_file_name[16];			// имя файла спецификаций (16)
};
// Итого, заголовок занимает 28 байт
#pragma pack(pop)
static_assert(sizeof(ItemsHeader) == 28, "ItemsHeader size mismatch");