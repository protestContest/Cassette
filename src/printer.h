#pragma once

#include "value.h"

#define UNDERLINE_START "\x1B[4m"
#define UNDERLINE_END   "\x1B[0m"

#define Error(...)  do { fprintf (stderr, __VA_ARGS__); exit(1); } while (0)

char *TypeAbbr(Value value);
void PrintValue(Value value);
void DebugValue(Value value);

typedef struct Table Table;

void EmptyTable(char *title);
void TableTitle(char *title, u32 num_cols, u32 cols_width);
#define BeginRow()    printf("\n  ")
#define TableSep()    printf(" │ ")
#define TableEnd()    printf("\n\n");

Table *BeginTable(char *title, u32 num_cols, ...);
void TableRow(Table *table, ...);
void EndTable(Table *table);

void HexDump(u8 *data, u32 size, char *title);
