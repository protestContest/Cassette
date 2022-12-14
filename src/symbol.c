#include "symbol.h"
#include "hash.h"

typedef struct Item {
  u32 hash;
  char *name;
  struct Item *next;
} Item;

Item *GetItem(u32 hash);
void PutItem(u32 hash, char *src, u32 len);

#define NUM_BUCKETS 32
static Item **symbols = NULL;
static u32 longest_sym_length = 0;

void InitSymbols(void)
{
  symbols = malloc(sizeof(Item **)*NUM_BUCKETS);
  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    symbols[i] = NULL;
  }
}

Value CreateSymbol(char *src, u32 len)
{
  u32 hash = Hash(src, len);
  Item *item = GetItem(hash);

  if (item == NULL) {
    PutItem(hash, src, len);
    if (len > longest_sym_length) {
      longest_sym_length = len;
    }
  }

  Value value = SymbolVal(hash);
  return value;
}

Item *GetItem(u32 hash)
{
  Item *item = symbols[hash % NUM_BUCKETS];
  while (item != NULL && item->hash != hash) item = item->next;
  return item;
}

void PutItem(u32 hash, char *src, u32 len)
{
  Item *item = malloc(sizeof(Item));
  item->hash = hash;
  item->name = malloc(len + 1);
  memcpy(item->name, src, len);
  item->name[len] = '\0';
  item->next = symbols[hash % NUM_BUCKETS];
  symbols[hash % NUM_BUCKETS] = item;
}

u32 LongestSymLength(void)
{
  return longest_sym_length;
}

char *SymbolName(Value sym)
{
  Item *item = GetItem(AsSymbol(sym));
  if (item == NULL) {
    fprintf(stderr, "No such symbol\n");
    exit(1);
  }
  return item->name;
}

void DumpSymbols(void)
{
  printf("  \x1B[4m★ Symbols             ");
  for (u32 i = 0; i < LongestSymLength(); i++) printf(" ");
  printf("\x1B[0m\n");
  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    Item *item = symbols[i];
    while (item != NULL) {
      printf("  % 4d │ 0x%8X │ \"%s\"\n", i, item->hash, item->name);
      item = item->next;
    }
  }
  printf("\n");
}


