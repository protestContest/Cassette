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
  FuncSection = 3,
  ExportSection = 7,
  StartSection = 8,
  CodeSection = 10
} SectionID;

void InitModule(Module *mod)
{
  InitVec(&mod->funcs, sizeof(void*), 0);
  InitVec(&mod->exports, sizeof(void*), 0);
}

void DestroyModule(Module *mod)
{
  u32 i;
  for (i = 0; i < mod->funcs.count; i++) {
    FreeFunc(VecRef(&mod->funcs, i));
  }
  DestroyVec(&mod->funcs);
  for (i = 0; i < mod->exports.count; i++) {
    FreeExport(VecRef(&mod->exports, i));
  }
  DestroyVec(&mod->exports);
}

Func *MakeFunc(u32 num_params)
{
  Func *func = Alloc(sizeof(Func));
  u32 i;
  InitVec(&func->params, sizeof(u8), num_params);
  for (i = 0; i < num_params; i++) ByteVecPush(&func->params, Int32);
  func->result = Int32;
  func->locals = 0;
  InitVec(&func->code, sizeof(u8), 0);
  return func;
}

void FreeFunc(Func *func)
{
  DestroyVec(&func->params);
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

static u32 IntSize(u32 n)
{
  if (n < Bit(7)) return 1;
  if (n < Bit(14)) return 2;
  if (n < Bit(21)) return 3;
  if (n < Bit(28)) return 4;
  return 5;
}

void PushInt(ByteVec *bytes, u32 n)
{
  u32 i;
  u8 byte;
  u32 size = IntSize(n);
  for (i = 0; i < size - 1; i++) {
    byte = ((n >> (7*i)) & 0x7F) | 0x80;
    ByteVecPush(bytes, byte);
  }
  byte = ((n >> (7*(size-1))) & 0x7F);
  ByteVecPush(bytes, byte);
}

void PushWord(ByteVec *bytes, u32 n)
{
  ByteVecPush(bytes, (n >> 0) & 0xFF);
  ByteVecPush(bytes, (n >> 8) & 0xFF);
  ByteVecPush(bytes, (n >> 16) & 0xFF);
  ByteVecPush(bytes, (n >> 24) & 0xFF);
}

static void AppendBytes(ByteVec *a, ByteVec *b)
{
  u8 *dst = a->items + a->count;
  GrowVec(a, sizeof(u8), b->count);
  Copy(b->items, dst, b->count);
}

static void PushHeader(ByteVec *bytes)
{
  ByteVecPush(bytes, 0);
  ByteVecPush(bytes, 'a');
  ByteVecPush(bytes, 's');
  ByteVecPush(bytes, 'm');
  PushWord(bytes, 1);
}

static void PushTypes(ByteVec *bytes, ObjVec *funcs)
{
  u32 i, j;
  u32 size = 0;
  if (funcs->count == 0) return;

  ByteVecPush(bytes, TypeSection);

  size += IntSize(funcs->count); /* vec size of functypes */
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);
    size++; /* functype label */
    size += IntSize(func->params.count); /* vec size of params */
    size += func->params.count; /* 1 byte for each param type */
    size += 1; /* vec size of results */
    size += 1; /* 1 byte for result type */
  }

  PushInt(bytes, size);
  PushInt(bytes, funcs->count);
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);
    ByteVecPush(bytes, 0x60);
    PushInt(bytes, func->params.count);
    for (j = 0; j < func->params.count; j++) {
      ByteVecPush(bytes, VecRef(&func->params, j));
    }
    PushInt(bytes, 1);
    ByteVecPush(bytes, func->result);
  }
}

static void PushFuncs(ByteVec *bytes, ObjVec *funcs)
{
  u32 i;
  u32 size = 0;
  if (funcs->count == 0) return;


  size += IntSize(funcs->count);
  for (i = 0; i < funcs->count; i++) {
    size += IntSize(i);
  }

  ByteVecPush(bytes, 0x03);
  PushInt(bytes, size);
  PushInt(bytes, funcs->count);
  for (i = 0; i < funcs->count; i++) {
    PushInt(bytes, i);
  }
}

static void PushCode(ByteVec *bytes, ObjVec *funcs)
{
  u32 i;
  u32 size = 0;
  u32 codesize = 0;
  if (funcs->count == 0) return;

  size += IntSize(funcs->count); /* count of entries */
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);

    if (func->locals == 0) {
      codesize += IntSize(0); /* count of locals vec */
    } else {
      codesize += IntSize(1); /* count of locals vec */
      codesize += IntSize(func->locals); /* count of locals of a particular type */
      codesize += 1; /* byte for local type */
    }
    codesize += func->code.count; /* size of code, plus end byte */


    size += IntSize(codesize); /* size of entry */
    size += codesize;
  }

  ByteVecPush(bytes, CodeSection);
  PushInt(bytes, size);
  PushInt(bytes, funcs->count);
  for (i = 0; i < funcs->count; i++) {
    Func *func = VecRef(funcs, i);

    PushInt(bytes, codesize);
    if (func->locals == 0) {
      PushInt(bytes, 0); /* count of locals */
    } else {
      PushInt(bytes, 1); /* count of locals */
      PushInt(bytes, func->locals);
      ByteVecPush(bytes, Int32);
    }
    AppendBytes(bytes, &func->code);
  }
}

static void PushExports(ByteVec *bytes, ObjVec *exports)
{
  u32 size = 0;
  u32 i, j;

  size += IntSize(exports->count);
  for (i = 0; i < exports->count; i++) {
    Export *export = VecRef(exports, i);
    u32 namelen = StrLen(export->name);
    size += IntSize(namelen);
    size += namelen;
    size += 1; /* export type */
    size += IntSize(i);
  }

  ByteVecPush(bytes, ExportSection);
  PushInt(bytes, size);
  PushInt(bytes, exports->count);
  for (i = 0; i < exports->count; i++) {
    Export *export = VecRef(exports, i);
    u32 namelen = StrLen(export->name);
    PushInt(bytes, namelen);
    for (j = 0; j < namelen; j++) {
      ByteVecPush(bytes, export->name[j]);
    }
    ByteVecPush(bytes, 0); /* function export type */
    PushInt(bytes, i);
  }
}

static void PushStart(ByteVec *bytes, u32 start_index)
{
  ByteVecPush(bytes, StartSection);
  PushInt(bytes, IntSize(start_index));
  PushInt(bytes, start_index);
}

ByteVec SerializeModule(Module *mod)
{
  ByteVec bytes;

  InitVec(&bytes, sizeof(u8), 256);
  PushHeader(&bytes);

  PushTypes(&bytes, &mod->funcs);
  PushFuncs(&bytes, &mod->funcs);
  PushExports(&bytes, &mod->exports);
  PushCode(&bytes, &mod->funcs);

  return bytes;
}
