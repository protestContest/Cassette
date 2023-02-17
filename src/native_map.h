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

void InitNativeMap(NativeMap *map);
void NativeMapPut(NativeMap *map, Val key, NativeFn impl);
NativeFn NativeMapGet(NativeMap *map, Val key);
