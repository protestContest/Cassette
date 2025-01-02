#include "univ/hexdump.h"
#include "univ/str.h"
#include "univ/math.h"

void HexDump(void *data, u32 size)
{
  u8 *bytes = data;
  u32 colwidth = 4;
  u32 cols = 6;
  u32 rowwidth = colwidth*cols;
  u32 nrows = size / (rowwidth);
  u32 extra = size % (rowwidth);
  u32 sizewidth = NumDigits(size, 10);
  u32 i, j;
  u32 titlelen = 0;

  printf("┌");
  for (i = 0; i < sizewidth; i++) printf("─");
  printf("┬");
  for (i = 0; i < 2*colwidth*cols + cols + 1 - titlelen; i++) printf("─");
  printf("┬");
  for (i = 0; i < rowwidth; i++) printf("─");
  printf("┐\n");

  for (i = 0; i < nrows; i++) {
    u8 *line = bytes;
    printf("│%*ld│ ", sizewidth, bytes - (u8*)data);
    for (j = 0; j < rowwidth; j++) {
      printf("%02X", bytes[j]);
      if (j % colwidth == colwidth-1) printf(" ");
    }
    bytes += rowwidth;
    printf("│");
    for (j = 0; j < rowwidth; j++) {
      if (line[j] < ' ') {
        printf(" ");
      } else if (line[j] >= 0x7F) {
        printf(".");
      } else {
        printf("%c", line[j]);
      }
    }
    printf("│\n");
  }

  printf("│%*ld│ ", sizewidth, bytes - (u8*)data);
  for (i = 0; i < rowwidth; i++) {
    if (i < extra) printf("%02X", bytes[i]);
    else printf("  ");
    if (i % colwidth == colwidth-1) printf(" ");
  }

  printf("│");
  for (j = 0; j < rowwidth; j++) {
    if (j < extra) {
      if (IsPrintable(bytes[j])) printf("%c", bytes[j]);
      else printf("·");
    } else {
      printf(" ");
    }
  }
  printf("│\n");

  printf("└");
  for (i = 0; i < sizewidth; i++) printf("─");
  printf("┴");
  printf("╴%d bytes╶", size);
  for (i = 0; i < 2*colwidth*cols + cols + 1 - sizewidth - 8; i++) printf("─");
  printf("┴");
  for (i = 0; i < rowwidth; i++) printf("─");
  printf("┘\n");
}
