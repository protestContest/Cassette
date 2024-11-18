#include "env.h"

Env **ExtendEnv(u32 size, Env **parent)
{
  u32 i;
  Env **env = New(Env);
  (*env)->size = size;
  (*env)->items = (u32**)NewHandle(sizeof(*(*env)->items)*size);
  (*env)->parent = parent;
  for (i = 0; i < size; i++) (*(*env)->items)[i] = 0;
  return env;
}

Env **PopEnv(Env **env)
{
  Env **parent;
  if (!env) return env;
  parent = (*env)->parent;
  DisposeHandle((Handle)(*env)->items);
  DisposeHandle((Handle)env);
  return parent;
}

EnvPosition EnvFind(u32 value, Env **env)
{
  EnvPosition pos = {0, 0};
  while (env) {
    u32 i;
    for (i = 0; i < (*env)->size; i++) {
      pos.index = (*env)->size - 1 - i;
      if ((*(*env)->items)[pos.index] == value) return pos;
    }
    pos.frame++;
    env = (*env)->parent;
  }
  pos.frame = -1;
  return pos;
}

bool EnvSet(u32 var, u32 index, Env **env)
{
  if (!env || index >= (*env)->size) return false;
  (*(*env)->items)[index] = var;
  return true;
}

u32 EnvGet(u32 index, Env **env)
{
  while (env) {
    if (index < (*env)->size) {
      return (*(*env)->items)[index];
    }
    index -= (*env)->size;
    env = (*env)->parent;
  }
  return 0;
}
