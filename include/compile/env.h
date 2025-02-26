#pragma once
#include "runtime/mem.h"

/*
 * An Env keeps track of variables currently in scope during compilation. Each item in the env is
 * the symbol of the variable and an optional constant value.
 */

typedef struct {
  u32 var;
  u32 value;
} EnvItem;

#define EnvUndefined ObjVal(1)

typedef struct Env {
  u32 size;
  EnvItem *items;
  struct Env *parent;
} Env;

Env *ExtendEnv(u32 size, Env *parent);
Env *PopEnv(Env *env);
i32 EnvFind(u32 var, Env *env);
bool EnvSet(u32 var, u32 value, u32 index, Env *env);
u32 EnvGet(u32 index, Env *env);
