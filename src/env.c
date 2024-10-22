#include "env.h"

Env *ExtendEnv(u32 size, Env *parent)
{
  u32 i;
  Env *env = malloc(sizeof(Env));
  env->size = size;
  env->items = malloc(size*sizeof(*env->items));
  env->parent = parent;
  for (i = 0; i < size; i++) env->items[i] = 0;
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
