#pragma once

u32 SourceLine(u32 pos, char *source);
u32 SourceCol(u32 pos, char *source);
void PrintSourceContext(u32 pos, char *source, u32 context);
