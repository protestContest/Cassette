#pragma once
#include "univ/vec.h"
#include "univ/result.h"
#include "ast.h"

typedef enum {
  ExternRef = 0x6F,
  FuncRef = 0x70,
  Flt64 = 0x7C,
  Flt32 = 0x7D,
  Int64 = 0x7E,
  Int32 = 0x7F
} ValType;

typedef struct {
  u32 params;
  u32 results;
} Type;

typedef struct {
  u32 type;
  u32 locals;
  ByteVec code;
} Func;

typedef struct {
  u32 type;
  char *mod;
  char *name;
} Import;

typedef struct {
  char *name;
  u32 index;
} Export;

typedef struct {
  ObjVec types;
  ObjVec imports;
  ObjVec funcs;
  ObjVec exports;
  u32 start;
  u32 num_globals;
  char *filename;
} Module;

void InitModule(Module *mod);
void DestroyModule(Module *mod);
Func *MakeFunc(u32 type, u32 locals);
void FreeFunc(Func *func);
Export *MakeExport(char *name, u32 index);
void FreeExport(Export *export);
u32 AddImport(Module *mod, char *name, u32 type);
u32 TypeIdx(Module *mod, u32 params, u32 results);
#define FuncIdx(mod, id)  ((id) + (mod)->imports.count)

ByteVec SerializeModule(Module *module);
void PushByte(ByteVec *bytes, u8 n);
void InsertByte(ByteVec *bytes, u32 index, u8 n);
void PushInt(ByteVec *bytes, u32 n);
void PushWord(ByteVec *bytes, u32 n);
