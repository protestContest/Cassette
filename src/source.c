#include "source.h"
#include "lex.h"

static bool AtEnd(Source src, u32 pos)
{
  return pos >= src.length || src.data[pos] == '\0';
}

void PrintTokenPosition(Source src, Token token)
{
  Print(src.name);
  Print(":");
  PrintInt(token.line);
  Print(":");
  PrintInt(token.col);
}

void PrintSourceLine(Source src, u32 line, u32 start, u32 length)
{
  if (line == 0) return;

  u32 cur_line = 1;
  u32 pos = 0;
  while (cur_line < line) if (src.data[pos++] == '\n') cur_line++;

  PrintIntN(cur_line, 4, ' ');
  Print("│ ");
  u32 col = 1;
  while (src.data[pos] != '\n' && src.data[pos] != '\0') {
    if (col == start) Print(IOUnderline);
    if (col == start + length) Print(IONoUnderline);
    PrintChar(src.data[pos++]);
    col++;
  }
  Print(IONoUnderline);
  Print("\n");
}

void PrintTokenContext(Source src, Token token, u32 num_lines)
{
  u32 token_pos = token.lexeme - src.data;

  u32 before = num_lines / 2;
  u32 after = num_lines - before - 1;
  if (before > token.line - 1) {
    before = token.line - 1;
    after = num_lines - before - 1;
  }

  u32 start_line = token.line - before;
  u32 end_line = token.line + after;

  u32 pos = 0;
  u32 cur_line = 1;

  // skip lines before start_line
  while (!AtEnd(src, pos) && cur_line < start_line) {
    if (src.data[pos] == '\n') cur_line++;
    pos++;
  }

  // print lines
  while (!AtEnd(src, pos) && cur_line <= end_line) {
    if (cur_line == token.line) {
      Print(" →");
    } else {
      Print("  ");
    }
    PrintIntN(cur_line, 4, ' ');
    Print("│ ");

    while (!AtEnd(src, pos)) {
      if (pos == token_pos) Print(IOUnderline);
      if (pos == token_pos + token.length) Print(IONoUnderline);

      PrintChar(src.data[pos]);
      if (src.data[pos] == '\n') {
        cur_line++;
        pos++;
        break;
      }

      pos++;
    }
  }

  if (AtEnd(src, pos)) Print("\n");
}