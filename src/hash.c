#include "hash.h"

u32 Hash(char *str, u32 size)
{
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
