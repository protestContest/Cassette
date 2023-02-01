#pragma once
#include "vm.h"

#define EnvVar(vm, pair)         Head((vm)->heap, pair)
#define EnvVal(vm, pair)         Tail((vm)->heap, pair)
#define SetEnvVal(vm, pair, val) SetTail(&(vm)->heap, pair, val)
#define FirstPair(vm, frame)     Head((vm)->heap, frame)
#define RestOfFrame(vm, frame)   Tail((vm)->heap, frame)
#define FirstFrame(vm, env)      Head((vm)->heap, env)
#define RestOfEnv(vm, env)       Tail((vm)->heap, env)

Val ExtendEnv(VM *vm, Val env);
void Define(VM *vm, Val var, Val val, Val env);
Result Lookup(VM *vm, Val var, Val env);
