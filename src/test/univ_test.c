#include "univ.h"
#include <stdio.h>
#include <assert.h>

static void TestHashMap(void)
{
  u32 i, j;
  HashMap map;
  InitHashMap(&map);

  for (i = 0; i < 1000; i++) {
    HashMapSet(&map, i, i*2);

    for (j = 0; j <= i; j++) {
      assert(HashMapGet(&map, j) == j*2);
    }
  }

  for (i = 0; i < 1000; i++) {
    if (i % 8 == 0) {
      HashMapSet(&map, i, i*3);
    } else {
      HashMapDelete(&map, i);
    }
  }

  for (i = 0; i < 1000; i++) {
    if (i % 8 == 0) {
      assert(HashMapContains(&map, i));
      assert(HashMapGet(&map, i) == i*3);
    } else {
      assert(!HashMapContains(&map, i));
    }
  }
}

static void TestPopCount(void)
{
  assert(PopCount(0xB00B1E5) == 12);
  assert(PopCount(0xD1CC) == 8);
}

void TestUniv(void)
{
  TestHashMap();
  TestPopCount();
}
