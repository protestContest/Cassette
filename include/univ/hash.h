#pragma once

#define EmptyHash 5381
u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, void *data, u32 size);
u32 FoldHash(u32 hash, i32 size);
