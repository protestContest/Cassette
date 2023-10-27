#include "univ.h"

#define HashPrime           ((u32)0x01000193)

u32 AppendHash(u32 base, void *data, u32 size)
{
    u32 hash = base;
    u32 i;
    for (i = 0; i < size; i++) {
        hash ^= ((u8*)data)[i];
        hash *= HashPrime;
    }
    return hash;
}

u32 Hash(void *data, u32 size)
{
    return AppendHash(EmptyHash, data, size);
}

u32 HashBits(void *data, u32 size, u32 num_bits)
{
    u32 hash = Hash(data, size);
    u32 mask = 0xFFFFFFFF >> (32 - size);
    return (hash >> size) ^ (hash & mask);
}
