#pragma once

u32 Compress(void *src, u32 srcLen, u8 **dst, u32 symbolSize);
u32 Decompress(void *src, u32 srcLen, u8 **dst, u32 symbolSize);
