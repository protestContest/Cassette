#pragma once
#include "univ/vec.h"
#include "univ/result.h"
#include "ast.h"

typedef enum {
  Flt64 = 0x7C,
  Flt32 = 0x7D,
  Int64 = 0x7E,
  Int32 = 0x7F
} Type;

typedef struct {
  ByteVec params;
  u32 locals;
  Type result;
  ByteVec code;
} Func;

typedef struct {
  char *name;
  u32 index;
} Export;

typedef struct {
  ObjVec funcs;
  ObjVec exports;
} Module;

void InitModule(Module *mod);
void DestroyModule(Module *mod);
Func *MakeFunc(u32 num_params);
void FreeFunc(Func *func);
Export *MakeExport(char *name, u32 index);
void FreeExport(Export *export);

ByteVec SerializeModule(Module *module);
void PushInt(ByteVec *bytes, u32 n);
void PushWord(ByteVec *bytes, u32 n);
