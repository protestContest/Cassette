#include "env.h"
#include <univ.h>

Env *ExtendEnv(Env *parent, u32 size)
{
  u32 i;
  Env *env = malloc(sizeof(Env));
  env->parent = parent;
  env->items = NewVec(u32, size);
  for (i = 0; i < size; i++) {
    VecPush(env->items, 0);
  }
  return env;
}

Env *PopEnv(Env *env)
{
  Env *parent = env->parent;
  FreeVec(env->items);
  free(env);
  return parent;
}

void FreeEnv(Env *env)
{
  while (env != 0) env = PopEnv(env);
}

void Define(u32 name, u32 index, Env *env)
{
  assert(index < VecCount(env->items));
  env->items[index] = name;
}

i32 Lookup(u32 var, Env *env)
{
  i32 index;
  for (index = (i32)VecCount(env->items) - 1; index >= 0; index--) {
    u32 item = env->items[index];
    if (item == var) return index;
  }

  if (env->parent == 0) return -1;
  index = Lookup(var, env->parent);
  if (index < 0) return index;
  return VecCount(env->items) + index;
}

i32 LookupFrame(u32 name, Env *env)
{
  u32 i;
  if (!env) return -1;

  for (i = 0; i < VecCount(env->items); i++) {
    if (env->items[i] == name) return EnvSize(env) - 1;
  }

  return LookupFrame(name, env->parent);
}

i32 EnvSize(Env *env)
{
  i32 length = 0;
  while (env) {
    length++;
    env = env->parent;
  }
  return length;
}

Env *GetEnv(i32 index, Env *env)
{
  i32 length = 0;
  i32 i;
  Env *cur = env;
  while (cur) {
    length++;
    cur = cur->parent;
  }

  for (i = 0; i < length - index - 1; i++) {
    env = env->parent;
  }
  return env;
}

void PrintEnv(Env *env)
{
  while (env) {
    u32 i;
    printf("- ");
    for (i = 0; i < VecCount(env->items); i++) {
      u32 var = env->items[i];
      printf("%s ", SymbolName(var));
    }
    printf("\n");
    env = env->parent;
  }
}

