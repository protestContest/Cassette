#include "source.h"

u32 LineNum(char *src, u32 pos)
{
  u32 line = 0;
  for (; *src != '\0' && pos > 0; src++, pos--) {
    if (*src == '\n') line++;
  }
  return line;
}

char *LineEnd(char *line)
{
  while (*line != '\0' && *line != '\n') line++;
  if (*line == '\n') line++;
  return line;
}

void PrintLine(u32 num, char *line)
{
  PrintIntN(num+1, 4);
  Print("│ ");
  PrintN(line, LineEnd(line) - line);
}

void PrintSourceContext(char *src, u32 pos)
{
  u32 lines_before = 5, lines_after = 5;
  u32 line_num = LineNum(src, pos);
  char *line = src;

  for (u32 i = 0; i < line_num; i++) {
    if (*line == '\0') break;
    if (line_num < lines_before || i >= line_num - lines_before) {
      PrintLine(i, line);
    }

    line = LineEnd(line);
  }

  u32 col = pos - (line - src);

  Print("*");
  PrintIntN(line_num+1, 3);
  Print("│ ");
  PrintN(line, LineEnd(line) - line);
  Print("      ");
  for (u32 i = 0; i < col; i++) Print(" ");
  Print("^\n");
  line = LineEnd(line);

  for (u32 i = line_num + 1; i < line_num + lines_after; i++) {
    if (*line == '\0') break;
    PrintLine(i, line);
    line = LineEnd(line);
  }
}

void PrintCompileError(CompileError *error)
{
  PrintEscape(IOFGRed);
  Print(error->message);
  Print("\n");
  if (error->file) {
    char *src = ReadFile(error->file);
    if (src) {
      PrintSourceContext(src, error->pos);
    }
  }
  PrintEscape(IOFGReset);
}
