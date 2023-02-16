#pragma once
#include "value.h"

typedef struct VM VM;

typedef void (*NativeFn)(VM *vm, u32 num_args);

typedef struct {
  Val name;
  NativeFn impl;
} NativeMapItem;

typedef struct {
  u32 count;
  u32 capacity;
  NativeMapItem *items;
} NativeMap;

void DefineNatives(VM *vm);
void DoNative(VM *vm, Val name, u32 num_args);
