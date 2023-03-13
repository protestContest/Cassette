#pragma once
#include "../value.h"

#define EnvVar(vm, pair)          Head((vm)->heap, pair)
#define EnvVal(vm, pair)          Tail((vm)->heap, pair)
#define SetEnvVal(vm, pair, val)  SetTail(&(vm)->heap, pair, val)
#define FirstPair(vm, frame)      Head((vm)->heap, frame)
#define RestOfFrame(vm, frame)    Tail((vm)->heap, frame)
#define FrameHeader(vm, env)      Head((vm)->heap, env)
#define FirstFrame(vm, env)       Tail((vm)->heap, FrameHeader(vm, env))
#define RestOfEnv(vm, env)        Tail((vm)->heap, env)

typedef struct StringMap StringMap;

Val ExtendEnv(Val **mem, Val env);
Val ParentEnv(Val *mem, Val env);
void Define(Val **mem, Val var, Val val, Val env);
Result Lookup(Val *mem, Val var, Val env);
Val FrameMap(Val *mem, Val env);
void PrintEnv(Val *mem, Val symbols, Val env);
