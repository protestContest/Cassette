#pragma once

u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, u8 byte);
u32 SkipHash(u32 hash, u8 byte, u32 size);
u32 FoldHash(u32 hash, u32 bits);
