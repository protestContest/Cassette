#include "env.h"
#include "univ/symbol.h"

Env *ExtendEnv(u32 size, Env *parent)
{
  Env *env = malloc(sizeof(Env));
  env->size = size;
  env->items = malloc(size*sizeof(*env->items));
  env->parent = parent;
  return env;
}

Env *PopEnv(Env *env)
{
  Env *parent;
  if (!env) return env;
  parent = env->parent;
  free(env->items);
  free(env);
  return parent;
}

EnvPosition EnvFind(u32 value, Env *env)
{
  EnvPosition pos = {0, 0};
  while (env) {
    for (pos.index = 0; pos.index < env->size; pos.index++) {
      if (env->items[pos.index] == value) return pos;
    }
    pos.frame++;
    env = env->parent;
  }
  pos.frame = -1;
  return pos;
}

bool EnvSet(u32 var, u32 index, Env *env)
{
  if (!env || index >= env->size) return false;
  env->items[index] = var;
  return true;
}

u32 EnvGet(u32 index, Env *env)
{
  while (env) {
    if (index < env->size) {
      return env->items[index];
    }
    index -= env->size;
    env = env->parent;
  }
  return 0;
}

void PrintEnv(Env *env)
{
  while (env) {
    u32 i;
    for (i = 0; i < env->size; i++) {
      printf("%s ", SymbolName(env->items[i]));
    }
    printf("\n");
    env = env->parent;
  }
}
