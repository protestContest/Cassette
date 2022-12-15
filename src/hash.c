#include "hash.h"

#define EMPTY_HASH ((u32)0x811c9dc5)
#define HASH_PRIME ((u32)0x01000193)

u32 Hash(void *obj, u32 size)
{
    char *str = (char*)obj;
    u32 result = EMPTY_HASH;
    char *current = str;
    char *end = current + size;

    u32 word = 0;
    while (current < end) {
        word <<= 8;
        word |= *current++;
        if ((current - str) % 4 == 0) {
            result ^= word;
            result *= HASH_PRIME;
            word = 0;
        }
    }
    if ((current - str) % 4 != 0) {
        result ^= word;
        result *= HASH_PRIME;
    }

    return result;
}
