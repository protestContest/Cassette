#include "symbol.h"
#include "hash.h"
#include "debug.h"

typedef struct Item {
  Value key;
  Value value;
  struct Item *next;
} Item;

bool HasKey(Value key);
Value MapGet(Value key);
void MapPut(Value key, Value value);

#define NUM_BUCKETS 32
static Item **symbols = NULL;
static u32 longest_sym_length = 0;
static u32 num_symbols = 0;

Value nil_val = 0;
Value true_val = 0;
Value false_val = 0;

void InitSymbols(void)
{
  symbols = malloc(sizeof(Item **)*NUM_BUCKETS);
  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    symbols[i] = NULL;
  }

  nil_val = CreateSymbol("nil", 0, 3);
  true_val = CreateSymbol("true", 0, 4);
  false_val = CreateSymbol("false", 0, 5);
}

void ResetSymbols(void)
{
  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    Item *item = symbols[i];
    while (item != NULL) {
      Item *next = item->next;
      free(item);
      item = next;
    }
  }
}

Value CreateSymbol(char *src, u32 start, u32 end)
{
  u32 len = (start >= end) ? 0 : end - start;
  Value symbol = SymbolVal(Hash(src + start, len));

  if (HasKey(symbol)) {
    return symbol;
  } else {
    Value binary = MakeBinary(src, start, end);
    MapPut(symbol, binary);
    if (len > longest_sym_length) {
      longest_sym_length = len;
    }
    num_symbols++;

    return symbol;
  }
}

bool HasKey(Value key)
{
  u32 hash = HashValue(key);
  Item *item = symbols[hash % NUM_BUCKETS];
  while (item != NULL && item->key != key) item = item->next;

  return item != NULL;
}

Value MapGet(Value key)
{
  u32 hash = HashValue(key);
  Item *item = symbols[hash % NUM_BUCKETS];
  while (item != NULL && item->key != key) item = item->next;

  if (item == NULL) {
    return nil_val;
  } else {
    return item->value;
  }
}

void MapPut(Value key, Value value)
{
  u32 hash = HashValue(key);
  Item *item = malloc(sizeof(Item));
  item->key = key;
  item->value = value;
  item->next = symbols[hash % NUM_BUCKETS];
  symbols[hash % NUM_BUCKETS] = item;
}

u32 LongestSymLength(void)
{
  return longest_sym_length;
}

Value SymbolName(Value sym)
{
  Value binary = MapGet(sym);
  if (IsNil(binary)) {
    fprintf(stderr, "No such symbol 0x%0X\n", sym);
    exit(1);
  }

  return binary;
}

void DumpSymbols(void)
{
  u32 table_width = LongestSymLength() + 2;
  DebugTable("★ Symbols", table_width, 1);

  if (num_symbols == 0) {
    DebugEmpty();
    return;
  }

  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    Item *item = symbols[i];
    while (item != NULL) {
      DebugRow();
      printf("  ");
      PrintValue(item->value, 0);
      item = item->next;
    }
  }
  EndDebugTable();
}
