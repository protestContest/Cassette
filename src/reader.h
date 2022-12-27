#pragma once
#include "vm.h"

typedef struct {
  u32 pos;
  Val token;
  Val root;
  char *src;
  VM *vm;
} Reader;

Val Read(VM *vm, char *src);
