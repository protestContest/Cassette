#pragma once

struct Token;

typedef struct {
  char *name;
  char *data;
  u32 length;
} Source;

Source ReadSourceFile(char *filename);
void PrintTokenPosition(Source src, struct Token token);
void PrintSourceLine(Source src, u32 line, u32 start, u32 length);
void PrintTokenContext(Source src, struct Token token, u32 num_lines);
