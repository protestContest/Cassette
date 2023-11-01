#include "source.h"
#include "lex.h"
#include "univ.h"
#include <stdio.h>

u32 SourceLine(u32 pos, char *source)
{
  char *cur = source;
  u32 line = 0;

  while (cur < source + pos) {
    if (*cur == '\n') line++;
    cur++;
  }

  return line;
}

u32 SourceCol(u32 pos, char *source)
{
  char *cur = source + pos;

  while (cur > source) {
    cur--;
    if (*cur == '\n') break;
  }
  return source + pos - cur - 1;
}

void PrintSourceContext(u32 pos, char *source, u32 context)
{
  Lexer lex;

  u32 line = SourceLine(pos, source);
  u32 col = SourceCol(pos, source);

  u32 start_line = Max((i32)line - (i32)context, 0);
  u32 end_line = line + context + 1;

  char *cur = source;
  u32 cur_line = 0, i;

  InitLexer(&lex, source, pos);

  while (cur_line < start_line) {
    if (*cur == '\n') cur_line++;
    cur++;
  }

  while (cur_line <= line) {
    printf("%3d│ ", cur_line+1);
    while (*cur != 0 && *cur != '\n') {
      printf("%c", *cur);
      cur++;
    }
    printf("\n");
    cur_line++;
    cur++;
  }

  printf("   │ ");
  for (i = 0; i < col; i++) printf(" ");
  for (i = 0; i < lex.token.length; i++) printf("^");
  printf("\n");

  while (cur_line < end_line) {
    printf("%3d│ ", cur_line+1);

    while (*cur != 0 && *cur != '\n') {
      printf("%c", *cur);
      cur++;
    }
    printf("\n");
    if (*cur == 0) break;
    cur_line++;
    cur++;
  }
}
