#include "dict.h"
#include "printer.h"

typedef struct DictEntry {
  Value key;
  Value value;
  struct DictEntry *next;
} DictEntry;

#define NUM_BUCKETS 32

void InitDict(Dict *dict)
{
  dict->items = malloc(sizeof(DictEntry **)*NUM_BUCKETS);
  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    dict->items[i] = NULL;
  }
}

bool DictHasKey(Dict *dict, Value key)
{
  u32 hash = ValueHash(key);
  DictEntry *item = dict->items[hash % NUM_BUCKETS];
  while (item != NULL && item->key != key) item = item->next;

  return item != NULL;
}

Value DictGet(Dict *dict, Value key)
{
  u32 hash = ValueHash(key);
  DictEntry *item = dict->items[hash % NUM_BUCKETS];
  while (item != NULL && item->key != key) item = item->next;

  if (item == NULL) {
    return nil_val;
  } else {
    return item->value;
  }
}

void DictPut(Dict *dict, Value key, Value value)
{
  u32 hash = ValueHash(key);
  DictEntry *item = malloc(sizeof(DictEntry));
  item->key = key;
  item->value = value;
  item->next = dict->items[hash % NUM_BUCKETS];
  dict->items[hash % NUM_BUCKETS] = item;
}

void DumpDict(Dict *dict, char *title, u32 width)
{
  Table *table = BeginTable(title, 1, width);

  for (u32 i = 0; i < NUM_BUCKETS; i++) {
    DictEntry *item = dict->items[i];
    while (item != NULL) {
      TableRow(table, item->value);
      item = item->next;
    }
  }
  EndTable(table);
}
