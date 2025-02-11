#include "runtime/stats.h"
#include "univ/time.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/hashmap.h"
#include "univ/vec.h"
#include <_time.h>

StatGroup *NewStatGroup(char *name)
{
  StatGroup *group = malloc(sizeof(StatGroup));
  group->name = name;
  group->stats = 0;
  InitHashMap(&group->map);
  return group;
}

void FreeStatGroup(StatGroup *group)
{
  FreeVec(group->stats);
  DestroyHashMap(&group->map);
  free(group);
}

void IncStat(StatGroup *group, char *name, u64 start)
{
  u64 end = Ticks();
  u32 hash = HashStr(name);
  u32 index;

  if (HashMapFetch(&group->map, hash, &index)) {
    group->stats[index].count++;
    group->stats[index].ticks += end - start;
  } else {
    Stat stat;
    stat.name = name;
    stat.count = 1;
    stat.ticks = end - start;
    VecPush(group->stats, stat);
    HashMapSet(&group->map, hash, VecCount(group->stats) - 1);
  }
}

void AdjustStat(StatGroup *group, char *name, i64 adjust)
{
  u32 hash = HashStr(name);
  u32 index;

  if (HashMapFetch(&group->map, hash, &index)) {
    if ((i64)group->stats[index].ticks < -adjust) {
      group->stats[index].ticks = 0;
    } else {
      group->stats[index].ticks += adjust;
    }
  }
}

u32 SizeAmount(u32 num)
{
  while (num > 1000) num /= 1000;
  return num;
}

char *SizeSuffix(u32 num)
{
  char *suffixes[] = {"", "k", "M", "G", "T"};
  u32 i = 0;
  while (num > 1000 && i < ArrayCount(suffixes)-1) {
    num /= 1000;
    i++;
  }
  return suffixes[i];
}

char *TimeSuffix(u32 num)
{
  char *suffixes[] = {"ns", "µs", "ms", "s"};
  u32 i = 0;
  while (num > 1000 && i < ArrayCount(suffixes)-1) {
    num /= 1000;
    i++;
  }
  return suffixes[i];
}

static void PrintCountField(u32 count, u32 size)
{
  u32 j;
  char *suffix = SizeSuffix(count);
  u32 printed = (u32)printf("%*d%s", size - StrLen(suffix), SizeAmount(count), suffix);
  for (j = printed; j < size; j++) printf(" ");
}

static void PrintTimeField(u32 count, u32 size)
{
  u32 j;
  char *suffix = TimeSuffix(count);
  u32 printed = (u32)printf("%*d%s", size - StrLen(suffix), SizeAmount(count), suffix);
  for (j = printed; j < size; j++) printf(" ");
}

void PrintStats(StatGroup *group)
{
  Stat *stats = group->stats;
  Stat total = {"Total", 0, 0};
  struct timespec tp;
  u32 res;
  u32 stat, col, printed, j;
  u32 avg, avg_time;
  u32 width = 0;
  u32 cols[5];
  u32 gutter = 2;

  char *headers[5] = {"Name", "Count", "Ticks", "Avg", "Time"};
  for (col = 0; col < 5; col++) cols[col] = StrLen(headers[col]);

  clock_getres(CLOCK_THREAD_CPUTIME_ID, &tp);
  res = tp.tv_nsec;

  /* Calculate column sizes & total */
  for (stat = 0; stat < VecCount(stats); stat++) {
    avg = stats[stat].ticks / stats[stat].count;
    avg_time = stats[stat].ticks*res/stats[stat].count;
    total.count += stats[stat].count;
    total.ticks += stats[stat].ticks;
    cols[0] = Max(cols[0], (u32)snprintf(0, 0, "%s", stats[stat].name));
    cols[1] = Max(cols[1], (u32)snprintf(0, 0, "%d%s", SizeAmount(stats[stat].count), SizeSuffix(stats[stat].count)));
    cols[2] = Max(cols[2], (u32)snprintf(0, 0, "%d%s", SizeAmount(stats[stat].ticks), SizeSuffix(stats[stat].ticks)));
    cols[3] = Max(cols[3], (u32)snprintf(0, 0, "%d%s", SizeAmount(avg), SizeSuffix(avg)));
    cols[4] = Max(cols[4], (u32)snprintf(0, 0, "%d%s", SizeAmount(avg_time), TimeSuffix(avg_time)));
  }
  avg = total.ticks / total.count;
  avg_time = total.ticks*res;
  cols[0] = Max(cols[0], (u32)snprintf(0, 0, "%s", total.name));
  cols[1] = Max(cols[1], (u32)snprintf(0, 0, "%d%s", SizeAmount(total.count), SizeSuffix(total.count)));
  cols[2] = Max(cols[2], (u32)snprintf(0, 0, "%d%s", SizeAmount(total.ticks), SizeSuffix(total.ticks)));
  cols[3] = Max(cols[3], (u32)snprintf(0, 0, "%d%s", SizeAmount(avg), SizeSuffix(avg)));
  cols[4] = Max(cols[4], (u32)snprintf(0, 0, "%d%s", SizeAmount(avg_time), TimeSuffix(avg_time)));

  printf("%s\n", group->name);

  for (col = 0; col < 5; col++) {
    printed = printf("%s", headers[col]);
    for (j = printed; j < cols[col] + gutter; j++) printf(" ");
    width += cols[col];
  }
  printf("\n");

  width += gutter * (ArrayCount(headers) - 1);
  for (j = 0; j < width; j++) printf("─");
  printf("\n");

  for (stat = 0; stat < VecCount(stats); stat++) {
    avg = stats[stat].ticks / stats[stat].count;
    avg_time = stats[stat].ticks*res/stats[stat].count;

    printed = (u32)printf("%s", stats[stat].name);
    for (j = printed; j < cols[0] + gutter; j++) printf(" ");
    PrintCountField(stats[stat].count, cols[1]);
    for (j = 0; j < gutter; j++) printf(" ");
    PrintCountField(stats[stat].ticks, cols[2]);
    for (j = 0; j < gutter; j++) printf(" ");
    PrintCountField(avg, cols[3]);
    for (j = 0; j < gutter; j++) printf(" ");
    PrintTimeField(avg_time, cols[4]);

    printf("\n");
  }

  for (j = 0; j < width; j++) printf("─");
  printf("\n");

  avg = total.ticks / total.count;
  avg_time = total.ticks*res;
  printed = (u32)printf("%s", total.name);
  for (j = printed; j < cols[0] + gutter; j++) printf(" ");

  PrintCountField(total.count, cols[1]);
  for (j = 0; j < gutter; j++) printf(" ");
  PrintCountField(total.ticks, cols[2]);
  for (j = 0; j < gutter; j++) printf(" ");
  PrintCountField(avg, cols[3]);
  for (j = 0; j < gutter; j++) printf(" ");
  PrintTimeField(avg_time, cols[4]);
  printf("\n");

  printf("\n");
}
