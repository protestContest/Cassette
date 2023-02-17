#pragma once

void *Allocate(u32 size);
void *Reallocate(void *ptr, u32 size);
void Free(void *ptr);
void MemCopy(void *dst, void *src, u32 size);
