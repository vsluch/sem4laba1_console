#pragma once
#include <cstdint>
using namespace std;

#pragma pack(push, 1)
struct SpecHeader {
	int32_t pointer_to_first_record;	 // указатель на логически первую запись списка (4)
	int32_t pointer_to_free_memory;		// указатель на свободную область файла (4)
};
// 8 байт всего
#pragma pack(pop)
static_assert(sizeof(SpecHeader) == 8, "SpecHeader size mismatch");