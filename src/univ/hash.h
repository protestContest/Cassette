#pragma once

#define EmptyHash           ((u32)0x97058A1C)

u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, void *data, u32 size);
u32 HashBits(void *data, u32 size, u32 num_bits);
