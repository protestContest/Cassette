#include "source.h"
#include "lex.h"
#include "univ.h"
#include <stdio.h>

void PrintSourceContext(u32 pos, char *source)
{
  char *cur = source;
  char *lines[5] = {0, 0, 0, 0, 0};
  u32 line_num = 0;
  u32 i;

  Lexer lex;
  InitLexer(&lex, source, pos);

  lines[4] = source;

  /* cursed algorithm */
  while (*cur != 0) {
    if (*cur == '\n') {
      lines[0] = lines[1];
      lines[1] = lines[2];
      lines[2] = lines[3];
      lines[3] = lines[4];
      lines[4] = cur+1;
      if (lines[3] > source+pos) break;
      if (cur < source+pos) line_num++;
    }
    cur++;
  }

  for (i = 0; i < 5; i++) {
    if (lines[i] == 0) continue;

    printf("%3d│ ", line_num+i-1);
    cur = lines[i];
    while (*cur != '\n') printf("%c", *cur++);
    printf("\n");

    if (lines[i] < source+pos && (i == 4 || lines[i+1] > source+pos)) {
      u32 col = source + pos - lines[i];
      u32 j;
      printf("     ");
      for (j = 0; j < col; j++) printf(" ");
      for (j = 0; j < lex.token.length; j++) printf("^");
      printf("\n");
    }
  }
}
