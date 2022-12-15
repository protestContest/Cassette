#include "hash.h"

#define FNV_BASIS           ((u32)0x97058A1C)
#define FNV_PRIME           ((u32)0x01000193)

u32 FNV(u8 *data, u32 size, u32 base)
{
    u32 hash = base;
    for (u32 i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

u32 Hash(void *data, u32 size)
{
    return FNV(data, size, FNV_BASIS);
}
