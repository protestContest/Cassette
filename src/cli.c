#include "cli.h"
#include "univ/str.h"
#include "univ/system.h"
#include "univ/math.h"
#include "compile/lex.h"
#include <stdio.h>

static void PrintSourceContext(u32 pos, char *source, u32 context);

int Usage(void)
{
  printf("Usage: cassette [<filename>]+\n");
  printf("       cassette -p <manifest file>\n");
  return 1;
}

void PrintError(Result error)
{
  u32 line, col;
  char *source = 0;

  if (error.filename) {
    source = ReadFile(error.filename);
    if (source) {
      line = LineNum(source, error.pos);
      col = ColNum(source, error.pos);
    }
  }

  printf("%s", ANSIRed);
  if (error.filename) {
    printf("%s", error.filename);
    if (source) {
      printf(":%d:%d", line+1, col+1);
    }
    printf(" ");
  }

  printf("Error: %s\n", error.error);
  if (source) {
    PrintSourceContext(error.pos, source, 3);
  }
  printf("%s", ANSINormal);
}

static void PrintSourceContext(u32 pos, char *source, u32 context)
{
  Lexer lex;
  u32 line, col, start_line, end_line, cur_line, i;
  char *cur;

  InitLexer(&lex, source, pos);
  line = LineNum(source, lex.token.lexeme - lex.source);
  col = ColNum(source, lex.token.lexeme - lex.source);

  start_line = Max((i32)line - (i32)context, 0);
  end_line = line + context + 1;

  cur = source;
  cur_line = 0;

  /* skip to first line of context */
  while (cur_line < start_line) {
    cur = LineEnd(cur);
    if (*cur == 0) break;
    cur_line++;
    cur++;
  }

  /* print lines up to and including target line */
  while (cur_line <= line) {
    char *end = LineEnd(cur);
    printf("%3d│ %.*s\n", cur_line+1, (u32)(end - cur), cur);
    cur = end+1;
    cur_line++;
    if (*end == 0) break;
  }

  /* underline target token */
  printf("   │ ");
  for (i = 0; i < col; i++) printf(" ");
  for (i = 0; i < lex.token.length; i++) printf("^");
  printf("\n");

  /* print remaining context */
  while (cur_line < end_line) {
    char *end = LineEnd(cur);
    printf("%3d│ %.*s\n", cur_line+1, (u32)(end - cur), cur);
    if (*end == 0) break;
    cur = end + 1;
    cur_line++;
  }
}
