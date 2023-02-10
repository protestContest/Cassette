#pragma once
#include "value.h"

typedef struct {
  Val key;
  u32 length;
  char *data;
} StringMapItem;

typedef struct {
  u32 count;
  u32 capacity;
  StringMapItem *items;
} StringMap;

void InitStringMap(StringMap *map);
void FreeStringMap(StringMap *map);

Val PutString(StringMap *map, char *str, u32 length);
u32 StringLength(StringMap *map, Val str);
char *StringData(StringMap *map, Val str);

Val MakeSymbol(StringMap *map, char *src);
Val MakeSymbolFromSlice(StringMap *map, char *src, u32 len);
Val SymbolFor(char *src);
char *SymbolName(StringMap *map, Val sym);
