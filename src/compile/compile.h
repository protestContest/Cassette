#pragma once
#include "scan.h"
#include "../image/image.h"

typedef struct Parser {
  Status status;
  struct {
    u32 line;
    u32 col;
    u32 cur;
    char *data;
  } source;
  Token token;
  char *error;
  Image *image;
  Module *module;
} Parser;

Status Compile(char *src, Image *image);
Status CompileModules(char *src, Image *image);
