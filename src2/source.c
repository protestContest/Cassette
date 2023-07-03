#include "source.h"

void PrintSourceContext(char *source, u32 line, u32 context_lines, u32 h_start, u32 h_length)
{
  if (line == 0) return;

  u32 line_start = (context_lines > line) ? 0 : line - context_lines;
  u32 line_end = line + context_lines + 1;

  u32 cur_line = 1;
  u32 pos = 0;
  while (source[pos] != '\0' && cur_line < line_start) {
    if (source[pos] == '\n') cur_line++;
    pos++;
  }

  // print lines before
  while (cur_line < line) {
    PrintIntN(cur_line, 4, ' ');
    Print("│ ");

    while (source[pos] != '\0' && source[pos] != '\n') {
      PrintChar(source[pos]);
      pos++;
    }

    Print("\n");
    cur_line++;
  }

  // print line
  Print("→");
  PrintIntN(cur_line, 3, ' ');
  Print("│ ");
  while (source[pos] != '\0' && source[pos] != '\n') {
    if (h_length > 0 && pos == h_start) PrintEscape(IOUnderline);
    if (h_length > 0 && pos == h_start + h_length) PrintEscape(IONoUnderline);
    PrintChar(source[pos]);
    pos++;
  }
  if (h_length > 0 && pos == h_start + h_length) PrintEscape(IONoUnderline);
  Print("\n");
  cur_line++;

  // print lines after
  while (cur_line < line_end) {
    PrintIntN(cur_line, 4, ' ');
    Print("| ");

    while (source[pos] != '\0' && source[pos] != '\n') {
      if (h_length > 0 && pos == h_start + h_length) PrintEscape(IONoUnderline);
      PrintChar(source[pos]);
      pos++;
    }

    Print("\n");
    cur_line++;
  }
}
