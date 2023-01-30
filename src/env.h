#pragma once
#include "value.h"

typedef struct Frame {
  Val *keys;
  Val *values;
  struct Frame *parent;
} Frame;

Frame *NewFrame(Frame *parent);
void FreeFrame(Frame *frame);

// Val InitialEnv(void);
// Val ExtendEnv(Val env, Val keys, Val vals);
// Val AddFrame(Val env, u32 size);
// void Define(Val name, Val value, Val env);
// void DefineModule(Val name, Val defs, Val env);

// Val GlobalEnv(Val env);
// EvalResult Lookup(Val var, Val env);

// void DumpEnv(Val env, bool all);
