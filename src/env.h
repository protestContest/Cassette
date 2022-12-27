#pragma once
#include "vm.h"

#define ParentEnv(vm, env)          Tail(vm, env)
#define CurFrame(vm, env)           Head(vm, env)
#define MakeFrame(vm, vars, vals)   MakePair(vm, vars, vals)
#define FrameVars(vm, frame)        Head(vm, frame)
#define FrameVals(vm, frame)        Tail(vm, frame)

Val InitialEnv(VM *vm);
void AddBinding(VM *vm, Val frame, Val var, Val val);
Val ExtendEnv(VM *vm, Val env, Val vars, Val vals);
Val Lookup(VM *vm, Val var, Val env);
void SetVar(VM *vm, Val var, Val val, Val env);
void Define(VM *vm, Val var, Val val, Val env);

