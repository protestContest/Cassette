#pragma once
#include "image.h"
#include "scan.h"
#include "image.h"

#define DEBUG_COMPILE 1

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
