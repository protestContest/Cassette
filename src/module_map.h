#pragma once
#include "value.h"
#include "module.h"

typedef struct {
  Val name;
  Module *module;
} ModuleMapItem;

typedef struct {
  u32 count;
  u32 capacity;
  ModuleMapItem *items;
} ModuleMap;

void InitModuleMap(ModuleMap *map);
void PutModule(ModuleMap *map, Val key, Module *module);
Module *GetModule(ModuleMap *map, Val key);
