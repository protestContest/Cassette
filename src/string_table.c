#include "string_table.h"

void InitStringTable(StringTable *table)
{
  InitMap(&table->symbols);
  table->positions = NULL;
  table->data = NULL;
}

void DestroyStringTable(StringTable *table)
{
  FreeVec(table->data);
  FreeVec(table->positions);
  DestroyMap(&table->symbols);
}

Val PutString(StringTable *table, char *str, u32 length)
{
  Val sym = SymbolFrom(str, length);
  if (!MapContains(&table->symbols, sym.as_v)) {
    u32 pos = VecCount(table->data);
    for (u32 i = 0; i < length; i++) {
      VecPush(table->data, str[i]);
    }
    u32 pos_num = VecCount(table->positions);
    VecPush(table->positions, pos);
    MapSet(&table->symbols, sym.as_v, pos_num);
  }
  return sym;
}

char *GetString(StringTable *table, Val symbol)
{
  Assert(IsSym(symbol));
  if (!MapContains(&table->symbols, symbol.as_v)) return NULL;

  u32 index = MapGet(&table->symbols, symbol.as_v);
  u32 pos = table->positions[index];
  return (char*)table->data + pos;
}

u32 GetStringLength(StringTable *table, Val symbol)
{
  Assert(IsSym(symbol));
  if (!MapContains(&table->symbols, symbol.as_v)) return 0;

  u32 index = MapGet(&table->symbols, symbol.as_v);
  u32 start = table->positions[index];
  u32 end;
  if (index == VecCount(table->positions) - 1) {
    end  = VecCount(table->data);
  } else {
    end = table->positions[index+1];
  }

  return end - start;
}

void PrintStringTable(StringTable *table)
{
  for (u32 i = 0; i < VecCount(table->positions); i++) {
    u32 start = table->positions[i];
    u32 end = (i + 1 < VecCount(table->positions)) ? table->positions[i+1] : VecCount(table->data);
    Print(":");
    PrintN((char*)table->data + start, end - start);
    Print("\n");
  }
}

