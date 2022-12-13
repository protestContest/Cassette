#pragma once

#define EMPTY_HASH ((u32)0x811c9dc5)
#define HASH_PRIME ((u32)0x01000193)

u32 Hash(char *data, u32 size);
