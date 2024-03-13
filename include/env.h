#pragma once

typedef struct Env {
  struct Env *parent;
  u32 *items;
} Env;

Env *ExtendEnv(Env *parent, u32 size);
Env *PopEnv(Env *env);
void FreeEnv(Env *env);
void Define(u32 name, u32 index, Env *env);
i32 Lookup(u32 name, Env *env);
i32 LookupFrame(u32 name, Env *env);
i32 EnvSize(Env *env);
Env *GetEnv(i32 index, Env *env);
void PrintEnv(Env *env);
