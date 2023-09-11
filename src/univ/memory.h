#pragma once

void *Allocate(u32 size);
void *Reallocate(void *ptr, u32 size);
void Free(void *ptr);
void Copy(void *src, void *dst, u32 size);
