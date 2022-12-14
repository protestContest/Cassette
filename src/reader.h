#pragma once

#include "mem.h"

typedef struct InputLine {
  char *data;
  struct InputLine *next;
} InputLine;

typedef struct {
  InputLine *first_line;
  InputLine *last_line;
} Input;

Value Read(Input *input);
Value Parse(char *src);
