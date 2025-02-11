#pragma once
#include "univ/hashmap.h"

typedef struct {
  char *name;
  u32 count;
  u32 ticks;
} Stat;

typedef struct {
  char *name;
  Stat *stats;
  HashMap map;
} StatGroup;

StatGroup *NewStatGroup(char *name);
void FreeStatGroup(StatGroup *group);
void IncStat(StatGroup *group, char *name, u64 start);
void AdjustStat(StatGroup *group, char *name, i64 adjust);
void PrintStats(StatGroup *group);
