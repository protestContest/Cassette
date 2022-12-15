#include "debug.h"
#include "string.h"
#include "mem.h"

#define MIN_TABLE_WIDTH 28
#define MAX_TABLE_WIDTH 140

void DebugTable(char *title, u32 width, u32 num_cols)
{
  u32 divider_width = (num_cols > 0) ? (num_cols - 1)*3 : 0;

  u32 table_width = width + divider_width;
  if (table_width < MIN_TABLE_WIDTH) table_width = MIN_TABLE_WIDTH;
  if (table_width > MAX_TABLE_WIDTH) table_width = MAX_TABLE_WIDTH;

  u32 title_width = StringLength(title, 1, 0);

  printf("  %s%s", UNDERLINE_START, title);
  for (u32 i = 0; i < table_width - title_width; i++) printf(" ");
  printf("%s", UNDERLINE_END);
}

void DebugValue(Value value, u32 len)
{
  printf("%s", TypeAbbr(value));
  if (len == 0) {
    printf(" ");
    PrintValue(value, 0);
  } else if (len > 2) {
    printf(" ");
    if (IsBinary(value)) {
      char fmt[16];
      sprintf(fmt, "0x%%0%dX", len-4);
      printf(fmt, ObjectAddr(value));
    } else {
      PrintValue(value, len - 2);
    }
  }
}
