#pragma once
#include "vec.h"

typedef struct {
  char *name;
  char *path;
} FontInfo;

ObjVec *ListFonts(void);
char *DefaultFont(void);
