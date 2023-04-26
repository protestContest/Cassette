#pragma once

struct Token;

typedef struct {
  char *name;
  char *data;
  u32 length;
} Source;

void PrintTokenPosition(Source src, struct Token token);
void PrintTokenContext(Source src, struct Token token, u32 num_lines);
