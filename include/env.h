#pragma once

/* An Env is a compile-time structure to keep track of variables currently in
 * scope
 */

typedef struct Env {
  u32 size;
  u32 *items;
  struct Env *parent;
} Env;

typedef struct {
  i32 frame;
  u32 index;
} EnvPosition;

Env *ExtendEnv(u32 size, Env *parent);
Env *PopEnv(Env *env);
EnvPosition EnvFind(u32 var, Env *env);
bool EnvSet(u32 var, u32 index, Env *env);
u32 EnvGet(u32 index, Env *env);
