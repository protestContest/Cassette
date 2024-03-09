#pragma once

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
  u8 *code;
  u32 size;
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
  Type **types;
  Import **imports;
  Func **funcs;
  u32 num_globals;
  Export **exports;
  u32 start;
  char *filename;
  struct {
    u32 types;
    u32 imports;
    u32 funcs;
    u32 exports;
    u32 elements;
  } size;
} Module;

void InitModule(Module *mod);
void DestroyModule(Module *mod);

u32 AddType(u32 params, u32 results, Module *mod);
u32 AddImport(char *modname, char *name, u32 type, Module *mod);
u32 ImportIdx(char *modname, char *name, Module *mod);
Func *AddFunc(u32 type, u32 locals, Module *mod);
u32 FuncIdx(Func *func, Module *mod);
Export *AddExport(char *name, u32 index, Module *mod);

u8 *SerializeModule(Module *module);
