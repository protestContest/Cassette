#pragma once

/* Functions for reading/writing LEB128 values */

i32 LEBSize(i32 num);
void WriteLEB(i32 num, u32 pos, u8 *buf);
i32 ReadLEB(u32 index, u8 *buf);
