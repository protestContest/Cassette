#pragma once
#include "vm.h"

typedef struct {
  u32 pos;
  Val token;
  char *src;
  VM *vm;
} Reader;

Val Read(VM *vm, char *src);
Val ReadFile(VM *vm, char *path);
