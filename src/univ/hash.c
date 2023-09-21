#include "hash.h"

#define HashPrime           ((u32)0x01000193)

static u32 FNV(u8 *data, u32 size, u32 base)
{
    u32 hash = base;
    u32 i;
    for (i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= HashPrime;
    }
    return hash;
}

u32 Hash(void *data, u32 size)
{
    return FNV(data, size, EmptyHash);
}

u32 AppendHash(u32 hash, void *data, u32 size)
{
    return FNV(data, size, hash);
}

u32 HashBits(void *data, u32 size, u32 num_bits)
{
    u32 hash = FNV(data, size, EmptyHash);
    u32 mask = 0xFFFFFFFF >> (32 - size);
    return (hash >> size) ^ (hash & mask);
}
