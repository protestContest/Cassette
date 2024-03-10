#include "module.h"
#include "wasm.h"
#include <univ.h>

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
  mod->types = 0;
  mod->imports = 0;
  mod->funcs = 0;
  mod->num_globals = 0;
  mod->exports = 0;
  mod->start = -1;
  mod->filename = 0;
  mod->size.types = 0;
  mod->size.imports = 0;
  mod->size.funcs = 0;
  mod->size.exports = 0;
  mod->size.elements = 0;
}

void DestroyModule(Module *mod)
{
  u32 i;
  for (i = 0; i < VecCount(mod->types); i++) {
    free(mod->types[i]);
  }
  for (i = 0; i < VecCount(mod->funcs); i++) {
    FreeVec(mod->funcs[i]->code);
    free(mod->funcs[i]);
  }
  FreeVec(mod->funcs);
  for (i = 0; i < VecCount(mod->exports); i++) {
    free(mod->exports[i]->name);
    free(mod->exports[i]);
  }
  FreeVec(mod->exports);
}

u32 AddType(u32 params, u32 results, Module *mod)
{
  u32 i;
  Type *type;

  for (i = 0; i < VecCount(mod->types); i++) {
    type = mod->types[i];
    if (type->params == params && type->results == results) return i;
  }

  type = malloc(sizeof(Type));
  type->params = params;
  type->results = results;
  VecPush(mod->types, type);

  mod->size.types += 1; /* functype label */
  mod->size.types += IntSize(params); /* vec size of params */
  mod->size.types += type->params; /* 1 byte for each param type */
  mod->size.types += IntSize(results); /* vec size of results */
  mod->size.types += type->results; /* 1 byte for result type */

  return VecCount(mod->types) - 1;
}

u32 AddImport(char *modname, char *name, u32 type, Module *mod)
{
  Import *import = malloc(sizeof(Import));
  u32 modlen = strlen(modname);
  u32 namelen = strlen(name);
  import->mod = CopyStr(modname, modlen);
  import->name = CopyStr(name, namelen);
  import->type = type;

  VecPush(mod->imports, import);

  mod->size.imports += IntSize(modlen);
  mod->size.imports += modlen;
  mod->size.imports += IntSize(namelen);
  mod->size.imports += namelen;
  mod->size.imports += 1;
  mod->size.imports+= IntSize(type);

  return VecCount(mod->imports) - 1;
}

u32 ImportIdx(char *modname, char *name, Module *mod)
{
  u32 i;

  for (i = 0; i < VecCount(mod->imports); i++) {
    if (StrEq(modname, mod->imports[i]->mod) &&
        StrEq(name, mod->imports[i]->name)) {
      return i;
    }
  }
  return 0;
}

Func *AddFunc(u32 type, u32 locals, Module *mod)
{
  Func *func = malloc(sizeof(Func));

  func->type = type;
  func->locals = locals;
  func->code = 0;
  VecPush(mod->funcs, func);

  mod->size.funcs += IntSize(type);
  mod->size.elements += IntSize(VecCount(mod->funcs) - 1);

  return func;
}

u32 FuncIdx(Func *func, Module *mod)
{
  u32 i;
  for (i = 0; i < VecCount(mod->funcs); i++) {
    if (func == mod->funcs[i]) {
      return i + VecCount(mod->imports);
    }
  }
  return 0;
}

Export *AddExport(char *name, u32 index, Module *mod)
{
  Export *export = malloc(sizeof(Export));
  u32 namelen = strlen(name);

  export->name = CopyStr(name, namelen);
  export->index = index;
  VecPush(mod->exports, export);

  mod->size.exports += IntSize(namelen);
  mod->size.exports += namelen;
  mod->size.exports += 1; /* export type */
  mod->size.exports += IntSize(VecCount(mod->exports) - 1);

  return export;
}

static void SerializeHeader(u8 **bytes)
{
  EmitByte(0, bytes);
  EmitByte('a', bytes);
  EmitByte('s', bytes);
  EmitByte('m', bytes);
  EmitByte(1, bytes);
  EmitByte(0, bytes);
  EmitByte(0, bytes);
  EmitByte(0, bytes);
}

static void SerializeTypes(u8 **bytes, Module *mod)
{
  u32 i, j;
  Type **types = mod->types;
  u32 size = mod->size.types;
  if (VecCount(types) == 0) return;

  size += IntSize(VecCount(types)); /* vec size of functypes */

  EmitByte(TypeSection, bytes);
  EmitInt(size, bytes);
  EmitInt(VecCount(types), bytes);
  for (i = 0; i < VecCount(types); i++) {
    Type *type = types[i];
    EmitByte(0x60, bytes);
    EmitInt(type->params, bytes);
    for (j = 0; j < type->params; j++) {
      EmitByte(Int32, bytes);
    }
    EmitInt(type->results, bytes);
    for (j = 0; j < type->results; j++) {
      EmitByte(Int32, bytes);
    }
  }
}

static void SerializeImports(u8 **bytes, Module *mod)
{
  u32 i;
  Import **imports = mod->imports;
  u32 size = mod->size.imports;

  size += IntSize(VecCount(imports));

  EmitByte(ImportSection, bytes);
  EmitInt(size, bytes);
  EmitInt(VecCount(imports), bytes);
  for (i = 0; i < VecCount(imports); i++) {
    u32 j;
    Import *import = imports[i];
    u32 modlen = strlen(import->mod);
    u32 namelen = strlen(import->name);

    EmitInt(modlen, bytes);
    for (j = 0; j < modlen; j++) {
      EmitByte(import->mod[j], bytes);
    }
    EmitInt(namelen, bytes);
    for (j = 0; j < namelen; j++) {
      EmitByte(import->name[j], bytes);
    }

    EmitByte(0, bytes); /* function import type */
    EmitInt(import->type, bytes);
  }
}

static void SerializeFuncs(u8 **bytes, Module *mod)
{
  u32 i;
  Func **funcs = mod->funcs;
  u32 size = mod->size.funcs;

  if (VecCount(funcs) == 0) return;

  size += IntSize(VecCount(funcs));

  EmitByte(FuncSection, bytes);
  EmitInt(size, bytes);
  EmitInt(VecCount(funcs), bytes);

  for (i = 0; i < VecCount(funcs); i++) {
    Func *func = funcs[i];
    EmitInt(func->type, bytes);
  }
}

static void SerializeTable(u8 **bytes, Module *mod)
{
  u32 size = IntSize(1) + 3;

  EmitByte(TableSection, bytes);
  EmitInt(size, bytes);
  EmitInt(1, bytes);
  EmitByte(FuncRef, bytes);
  EmitByte(0, bytes); /* limit type (no max) , bytes*/
  EmitInt(VecCount(mod->funcs), bytes); /* min , bytes*/
}

static void SerializeMemory(u8 **bytes, Module *mod)
{
  u32 size = IntSize(1) + 1 + IntSize(1);
  EmitByte(MemorySection, bytes);
  EmitInt(size, bytes);
  EmitInt(1, bytes);
  EmitByte(0, bytes);
  EmitInt(1, bytes);
}

static void SerializeGlobals(u8 **bytes, Module *mod)
{
  u32 i;
  u32 size;

  size = IntSize(mod->num_globals) + 5*mod->num_globals;

  EmitByte(GlobalSection, bytes);
  EmitInt(size, bytes);
  EmitInt(mod->num_globals, bytes);
  for (i = 0; i < mod->num_globals; i++) {
    EmitByte(Int32, bytes); /* type , bytes*/
    EmitByte(1, bytes); /* var , bytes*/
    EmitByte(I32Const, bytes);
    EmitByte(0, bytes);
    EmitByte(EndOp, bytes);
  }
}

static void SerializeExports(u8 **bytes, Module *mod)
{
  u32 i, j;
  Export **exports = mod->exports;
  u32 size = mod->size.exports;

  if (VecCount(exports) == 0) return;

  size += IntSize(VecCount(exports));

  EmitByte(ExportSection, bytes);
  EmitInt(size, bytes);
  EmitInt(VecCount(exports), bytes);
  for (i = 0; i < VecCount(exports); i++) {
    Export *export = exports[i];
    u32 namelen = strlen(export->name);
    EmitInt(namelen, bytes);
    for (j = 0; j < namelen; j++) {
      EmitByte(export->name[j], bytes);
    }
    EmitByte(0, bytes); /* function export type , bytes*/
    EmitInt(i, bytes);
  }
}

static void SerializeStart(u8 **bytes, Module *mod)
{
  if (mod->start > VecCount(mod->funcs) + VecCount(mod->imports)) return;
  EmitByte(StartSection, bytes);
  EmitInt(IntSize(mod->start), bytes);
  EmitInt(mod->start, bytes);
}

static void SerializeElements(u8 **bytes, Module *mod)
{
  u32 i;
  Func **funcs = mod->funcs;
  u32 size = mod->size.elements;

  size += IntSize(1); /* num segments */
  size += 4; /* segment attrs */
  size += IntSize(VecCount(funcs));

  EmitByte(ElementsSection, bytes);
  EmitInt(size, bytes);
  EmitInt(1, bytes);

  EmitByte(0, bytes); /* mode 0, active , bytes*/
  EmitByte(I32Const, bytes); /* offset expr , bytes*/
  EmitInt(0, bytes);
  EmitByte(EndOp, bytes);
  EmitInt(VecCount(funcs), bytes); /* vec(funcidx) , bytes*/
  for (i = 0; i < VecCount(funcs); i++) {
    EmitInt(i, bytes);
  }
}

static void SerializeCode(u8 **bytes, Module *mod)
{
  u32 i;
  Func **funcs = mod->funcs;
  u32 size = 0;

  if (VecCount(funcs) == 0) return;

  size += IntSize(VecCount(funcs)); /* count of entries */
  for (i = 0; i < VecCount(funcs); i++) {
    Func *func = funcs[i];
    func->size = 0;
    if (func->locals > 0) {
      func->size += IntSize(1);
      func->size += IntSize(func->locals);
      func->size++; /* valtype of locals */
    } else {
      func->size += IntSize(0);
    }
    func->size += VecCount(func->code); /* size of code */
    size += IntSize(func->size);
    size += func->size;
  }

  EmitByte(CodeSection, bytes);
  EmitInt(size, bytes);
  EmitInt(VecCount(funcs), bytes);
  for (i = 0; i < VecCount(funcs); i++) {
    Func *func = funcs[i];
    EmitInt(func->size, bytes);

    if (func->locals > 0) {
      EmitInt(1, bytes);
      EmitInt(func->locals, bytes);
      EmitByte(Int32, bytes);
    } else {
      EmitInt(0, bytes);
    }

    AppendBytes(bytes, &func->code);
  }
}

u8 *SerializeModule(Module *mod)
{
  u8 *bytes = 0;

  SerializeHeader(&bytes);
  SerializeTypes(&bytes, mod);
  SerializeImports(&bytes, mod);
  SerializeFuncs(&bytes, mod);
  SerializeTable(&bytes, mod);
  SerializeMemory(&bytes, mod);
  SerializeGlobals(&bytes, mod);
  SerializeExports(&bytes, mod);
  SerializeStart(&bytes, mod);
  SerializeElements(&bytes, mod);
  SerializeCode(&bytes, mod);

  return bytes;
}
