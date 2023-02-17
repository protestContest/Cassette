#pragma once
#include "module_map.h"
#include "string.h"
#include "value.h"

typedef struct {
  char header[4];
  ModuleMap modules;
  Val *heap;
  Val env;
  StringMap strings;
} Image;

void InitImage(Image *image);
void WriteImage(Image *image, char *path);
Image *ReadImage(char *path);
