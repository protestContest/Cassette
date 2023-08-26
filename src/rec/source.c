#include "source.h"

void PrintSourceContext(char *src, u32 pos)
{
  u32 line = 0;
  u32 col = 0;
  char *line_start = src;
  for (u32 i = 0; i < pos && src[i] != '\0'; i++) {
    if (src[i] == '\n') {
      line++;
      line_start = src + i + 1;
      col = 0;
    } else {
      col++;
    }
  }
  char *line_end = line_start;
  while (*line_end != '\0' && *line_end != '\n') line_end++;

  PrintIntN(line+1, 4);
  Print("│ ");
  PrintN(line_start, line_end - line_start);
  Print("\n");
  Print("      ");
  for (u32 i = 0; i < col; i++) Print(" ");
  Print("^\n");
}

void PrintCompileError(CompileError *error, char *src)
{
  Print(error->message);
  Print("\n");
  PrintSourceContext(src, error->pos);
}
