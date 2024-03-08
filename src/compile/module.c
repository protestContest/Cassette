#include "module.h"
#include "parse.h"
#include "univ/system.h"
#include "univ/str.h"
#include "univ/math.h"
#include "env.h"
#include "ops.h"
#include "univ/hash.h"
#include <stdio.h>

typedef enum {
  TypeSection = 1,
  ImportSection = 2,
  FuncSection = 3,
  TableSection = 4,
  MemorySection = 5,
  GlobalSection = 6,
  ExportSection = 7,
  StartSection = 8,
  ElementsSection = 9,
  CodeSection = 10
} SectionID;

void InitModule(Module *mod)
{
  InitVec(&mod->types, sizeof(void*), 0);
  InitVec(&mod->imports, sizeof(void*), 0);
  InitVec(&mod->funcs, sizeof(void*), 0);
  InitVec(&mod->exports, sizeof(void*), 0);
  mod->num_globals = 0;
  mod->start = -1;
  mod->filename = 0;
}

void DestroyModule(Module *mod)
{
  u32 i;
  for (i = 0; i < mod->types.count; i++) {
    Free(VecRef(&mod->types, i));
  }
  for (i = 0; i < mod->funcs.count; i++) {
    FreeFunc(VecRef(&mod->funcs, i));
  }
  DestroyVec(&mod->funcs);
  for (i = 0; i < mod->exports.count; i++) {
    FreeExport(VecRef(&mod->exports, i));
  }
  DestroyVec(&mod->exports);
}

Func *MakeFunc(u32 type, u32 locals)
{
  Func *func = Alloc(sizeof(Func));
  func->type = type;
  func->locals = locals;
  InitVec(&func->code, sizeof(u8), 0);
  return func;
}

void FreeFunc(Func *func)
{
  Free(func);
}

Export *MakeExport(char *name, u32 index)
{
  Export *export = Alloc(sizeof(Export));
  export->name = CopyStr(name, StrLen(name));
  export->index = index;
  return export;
}

void FreeExport(Export *export)
{
  Free(export->name);
  Free(export);
}

u32 AddImport(Module *mod, char *name, u32 type)
{
  Import *import = Alloc(sizeof(Import));
  u32 i;
  u32 len = StrLen(name);
  import->mod = 0;
  import->name = 0;
  import->type = type;

  for (i = 0; i < len; i++) {
    if (name[i] == '.') {
      import->mod = Alloc(i + 1);
      Copy(name, import->mod, i);
      import->mod[i] = 0;
      i++;
      break;
    }
  }

  if (!import->mod) {
    import->mod = "imports";
    i = 0;
  }
  len -= i;
  import->name = Alloc(len+1);
  Copy(name+i, import->name, len);
  import->name[len] = 0;

  ObjVecPush(&mod->imports, import);

  return mod->imports.count - 1;
}

u32 TypeIdx(Module *mod, u32 params, u32 results)
{
  u32 i;
  Type *type;
  for (i = 0; i < mod->types.count; i++) {
    type = VecRef(&mod->types, i);
    if (type->params == params && type->results == results) return i;
  }

  type = Alloc(sizeof(Type));
  type->params = params;
  type->results = results;
  ObjVecPush(&mod->types, type);
  return mod->types.count - 1;
}

static u32 IntSize(u32 n)
{
  if (n < Bit(7)) return 1;
  if (n < Bit(14)) return 2;
  if (n < Bit(21)) return 3;
  if (n < Bit(28)) return 4;
  return 5;
}

void PushByte(ByteVec *bytes, u8 n)
{
  ByteVecPush(bytes, n);
}

void InsertByte(ByteVec *bytes, u32 index, u8 n)
{
  u32 i;
  GrowVec(bytes, sizeof(u8), 1);
  for (i = 0; i < bytes->count - index - 1; i++) {
    VecRef(bytes, index + i + 1) = VecRef(bytes, index + i);
  }
  VecRef(bytes, index) = n;
}

void PushInt(ByteVec *bytes, u32 n)
{
  u32 i;
  u8 byte;
  u32 size = IntSize(n);
  for (i = 0; i < size - 1; i++) {
    byte = ((n >> (7*i)) & 0x7F) | 0x80;
    PushByte(bytes, byte);
  }
  byte = ((n >> (7*(size-1))) & 0x7F);
  PushByte(bytes, byte);
}

void PushWord(ByteVec *bytes, u32 n)
{
  PushByte(bytes, (n >> 0) & 0xFF);
  PushByte(bytes, (n >> 8) & 0xFF);
  PushByte(bytes, (n >> 16) & 0xFF);
  PushByte(bytes, (n >> 24) & 0xFF);
}

static void AppendBytes(ByteVec *a, ByteVec *b)
{
  u32 a_count = a->count;
  GrowVec(a, sizeof(u8), b->count);
  Copy(b->items, a->items + a_count, b->count);
}

static void PushHeader(ByteVec *bytes)
{
  PushByte(bytes, 0);
  PushByte(bytes, 'a');
  PushByte(bytes, 's');
  PushByte(bytes, 'm');
  PushWord(bytes, 1);
}

static void PushTypes(ByteVec *bytes, ObjVec *types)
{
  u32 i, j;
  u32 size = 0;
  if (types->count == 0) return;

  size += IntSize(types->count); /* vec size of functypes */
  for (i = 0; i < types->count; i++) {
    Type *type = VecRef(types, i);
    size++; /* functype label */
    size += IntSize(type->params); /* vec size of params */
    size += type->params; /* 1 byte for each param type */
    size += IntSize(type->results); /* vec size of results */
    size += type->results; /* 1 byte for result type */
  }

  PushByte(bytes, TypeSection);
  PushInt(bytes, size);
  PushInt(bytes, types->count);
  for (i = 0; i < types->count; i++) {
    Type *type = VecRef(types, i);
    PushByte(bytes, 0x60);
    PushInt(bytes, type->params);
    for (j = 0; j < type->params; j++) {
      PushByte(bytes, Int32);
    }
    PushInt(bytes, type->results);
    for (j = 0; j < type->results; j++) {
      PushByte(bytes, Int32);
    }
  }
}

static void PushImports(ByteVec *bytes, ObjVec *imports)
{
  u32 i;
  u32 size = 0;

  size += IntSize(imports->count);
  for (i = 0; i < imports->count; i++) {
    Import *import = VecRef(imports, i);
    u32 modlen = StrLen(import->mod);
    u32 namelen = StrLen(import->name);

    size += IntSize(modlen);
    size += modlen;
    size += IntSize(namelen);
    size += namelen;
    size++;
    size+= IntSize(import->type);
  }

  PushByte(bytes, ImportSection);
  PushInt(bytes, size);
  PushInt(bytes, imports->count);
  for (i = 0; i < imports->count; i++) {
    u32 j;
    Import *import = VecRef(imports, i);
    u32 modlen = StrLen(import->mod);
    u32 namelen = StrLen(import->name);

    PushInt(bytes, modlen);
    for (j = 0; j < modlen; j++) {
      PushByte(bytes, import->mod[j]);
    }
    PushInt(bytes, namelen);
    for (j = 0; j < namelen; j++) {
      PushByte(bytes, import->name[j]);
    }

    PushByte(bytes, 0); /* function import type */
    PushInt(bytes, import->type);
  }
}

static void PushFuncs(ByteVec *bytes, Module *mod)
{
  u32 i;
  u32 size = 0;
  if (mod->funcs.count == 0) return;

  size += IntSize(mod->funcs.count);
  for (i = 0; i < mod->funcs.count; i++) {
    Func *func = VecRef(&mod->funcs, i);
    size += IntSize(func->type);
  }

  PushByte(bytes, FuncSection);
  PushInt(bytes, size);
  PushInt(bytes, mod->funcs.count);

  for (i = 0; i < mod->funcs.count; i++) {
    Func *func = VecRef(&mod->funcs, i);
    PushInt(bytes, func->type);
  }
}

static void PushTable(ByteVec *bytes, u32 min)
{
  u32 size = IntSize(1) + 3;

  PushByte(bytes, TableSection);
  PushInt(bytes, size);
  PushInt(bytes, 1);
  PushByte(bytes, FuncRef);
  PushByte(bytes, 0); /* limit type */
  PushInt(bytes, min); /* min */
}

static void PushMemory(ByteVec *bytes)
{
  u32 size = IntSize(1) + 1 + IntSize(1);
  PushByte(bytes, MemorySection);
  PushInt(bytes, size);
  PushInt(bytes, 1);
  PushByte(bytes, 0);
  PushInt(bytes, 1);
}

static void PushGlobals(ByteVec *bytes, u32 num_globals)
{
  u32 i;
  u32 size = 0;

  size += IntSize(num_globals);
  for (i = 0; i < num_globals; i++) {
    size += 1; /* type */
    size += 1; /* mut */
    size += 1; /* i32.const */
    size += 1; /* 0 */
    size += 1; /* end */
  }

  PushByte(bytes, GlobalSection);
  PushInt(bytes, size);
  PushInt(bytes, num_globals);
  for (i = 0; i < num_globals; i++) {
    PushByte(bytes, Int32); /* type */
    PushByte(bytes, 1); /* var */
    PushByte(bytes, I32Const);
    PushByte(bytes, 0);
    PushByte(bytes, EndOp);
  }
}

static void PushExports(ByteVec *bytes, ObjVec *exports)
{
  u32 size = 0;
  u32 i, j;

  if (exports->count == 0) return;

  size += IntSize(exports->count);
  for (i = 0; i < exports->count; i++) {
    Export *export = VecRef(exports, i);
    u32 namelen = StrLen(export->name);
    size += IntSize(namelen);
    size += namelen;
    size += 1; /* export type */
    size += IntSize(i);
  }

  PushByte(bytes, ExportSection);
  PushInt(bytes, size);
  PushInt(bytes, exports->count);
  for (i = 0; i < exports->count; i++) {
    Export *export = VecRef(exports, i);
    u32 namelen = StrLen(export->name);
    PushInt(bytes, namelen);
    for (j = 0; j < namelen; j++) {
      PushByte(bytes, export->name[j]);
    }
    PushByte(bytes, 0); /* function export type */
    PushInt(bytes, i);
  }
}

static void PushStart(ByteVec *bytes, Module *mod)
{
  if (mod->start > mod->funcs.count + mod->imports.count) return;
  PushByte(bytes, StartSection);
  PushInt(bytes, IntSize(mod->start));
  PushInt(bytes, mod->start);
}

static void PushElements(ByteVec *bytes, ObjVec *funcs)
{
  u32 i;
  u32 size = 0;

  size += IntSize(1); /* num segments */
  size += 4; /* segment attrs */
  size += IntSize(funcs->count);
  for (i = 0; i < funcs->count; i++) {
    size += IntSize(i);
  }

  PushByte(bytes, ElementsSection);
  PushInt(bytes, size);
  PushInt(bytes, 1);

  PushByte(bytes, 0); /* mode 0, active */
  PushByte(bytes, I32Const); /* offset expr */
  PushInt(bytes, 0);
  PushByte(bytes, EndOp);
  PushInt(bytes, funcs->count); /* vec(funcidx) */
  for (i = 0; i < funcs->count; i++) {
    PushInt(bytes, i);
  }
}

static void PushCode(ByteVec *bytes, ObjVec *funcs)
{
  u32 i;
  u32 size = 0;
  if (funcs->count == 0) return;

  size += IntSize(funcs->count); /* count of entries */
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);
    u32 codesize = 0;
    codesize = 0;
    if (func->locals > 0) {
      codesize += IntSize(1);
      codesize += IntSize(func->locals);
      codesize++; /* valtype of locals */
    } else {
      codesize += IntSize(0);
    }
    codesize += func->code.count; /* size of code */

    size += IntSize(codesize); /* size of entry */
    size += codesize;
  }

  PushByte(bytes, CodeSection);
  PushInt(bytes, size);
  PushInt(bytes, funcs->count);
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);
    u32 codesize = 0;
    if (func->locals > 0) {
      codesize += IntSize(1);
      codesize += IntSize(func->locals);
      codesize++; /* valtype of locals */
    } else {
      codesize += IntSize(0);
    }
    codesize += func->code.count; /* size of code */

    PushInt(bytes, codesize);

    if (func->locals > 0) {
      PushInt(bytes, 1);
      PushInt(bytes, func->locals);
      PushByte(bytes, Int32);
    } else {
      PushInt(bytes, 0);
    }

    AppendBytes(bytes, &func->code);
  }
}

ByteVec SerializeModule(Module *mod)
{
  ByteVec bytes;

  InitVec(&bytes, sizeof(u8), 256);
  PushHeader(&bytes);

  PushTypes(&bytes, &mod->types);
  PushImports(&bytes, &mod->imports);
  PushFuncs(&bytes, mod);
  PushTable(&bytes, mod->funcs.count);
  PushMemory(&bytes);
  PushGlobals(&bytes, mod->num_globals);
  PushExports(&bytes, &mod->exports);
  PushStart(&bytes, mod);
  PushElements(&bytes, &mod->funcs);
  PushCode(&bytes, &mod->funcs);

  return bytes;
}
