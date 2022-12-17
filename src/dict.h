#pragma once
#include "value.h"

typedef struct DictEntry DictEntry;

typedef struct Dict {
  DictEntry **items;
} Dict;

void InitDict(Dict *dict);
bool DictHasKey(Dict *dict, Value key);
Value DictGet(Dict *dict, Value key);
void DictPut(Dict *dict, Value key, Value value);

void DumpDict(Dict *dict, char *title, u32 width);
