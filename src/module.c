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


u32 AddImport(char *name, u32 type, Module *mod)
{
  Import *import = malloc(sizeof(Import));
  u32 i;
  u32 len = strlen(name);
  u32 modlen;
  import->mod = 0;
  import->name = 0;
  import->type = type;

  for (i = 0; i < len; i++) {
    if (name[i] == '.') {
      modlen = i;
      import->mod = malloc(i + 1);
      Copy(name, import->mod, i);
      import->mod[i] = 0;
      i++;
      break;
    }
  }

  if (!import->mod) {
    import->mod = "imports";
    modlen = 7;
    i = 0;
  }
  len -= i;
  import->name = malloc(len+1);
  Copy(name+i, import->name, len);
  import->name[len] = 0;

  VecPush(mod->imports, import);

  mod->size.imports += IntSize(modlen);
  mod->size.imports += modlen;
  mod->size.imports += IntSize(len);
  mod->size.imports += len;
  mod->size.imports += 1;
  mod->size.imports+= IntSize(type);

  return VecCount(mod->imports) - 1;
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
  PushByte(bytes, 0);
  PushByte(bytes, 'a');
  PushByte(bytes, 's');
  PushByte(bytes, 'm');
  PushByte(bytes, 1);
  PushByte(bytes, 0);
  PushByte(bytes, 0);
  PushByte(bytes, 0);
}

static void SerializeTypes(u8 **bytes, Module *mod)
{
  u32 i, j;
  Type **types = mod->types;
  u32 size = mod->size.types;
  if (VecCount(types) == 0) return;

  size += IntSize(VecCount(types)); /* vec size of functypes */

  PushByte(bytes, TypeSection);
  PushInt(bytes, size);
  PushInt(bytes, VecCount(types));
  for (i = 0; i < VecCount(types); i++) {
    Type *type = types[i];
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

static void SerializeImports(u8 **bytes, Module *mod)
{
  u32 i;
  Import **imports = mod->imports;
  u32 size = mod->size.imports;

  size += IntSize(VecCount(imports));

  PushByte(bytes, ImportSection);
  PushInt(bytes, size);
  PushInt(bytes, VecCount(imports));
  for (i = 0; i < VecCount(imports); i++) {
    u32 j;
    Import *import = imports[i];
    u32 modlen = strlen(import->mod);
    u32 namelen = strlen(import->name);

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

static void SerializeFuncs(u8 **bytes, Module *mod)
{
  u32 i;
  Func **funcs = mod->funcs;
  u32 size = mod->size.funcs;

  if (VecCount(funcs) == 0) return;

  size += IntSize(VecCount(funcs));

  PushByte(bytes, FuncSection);
  PushInt(bytes, size);
  PushInt(bytes, VecCount(funcs));

  for (i = 0; i < VecCount(funcs); i++) {
    Func *func = funcs[i];
    PushInt(bytes, func->type);
  }
}

static void SerializeTable(u8 **bytes, Module *mod)
{
  u32 size = IntSize(1) + 3;

  PushByte(bytes, TableSection);
  PushInt(bytes, size);
  PushInt(bytes, 1);
  PushByte(bytes, FuncRef);
  PushByte(bytes, 0); /* limit type (no max) */
  PushInt(bytes, VecCount(mod->funcs)); /* min */
}

static void SerializeMemory(u8 **bytes, Module *mod)
{
  u32 size = IntSize(1) + 1 + IntSize(1);
  PushByte(bytes, MemorySection);
  PushInt(bytes, size);
  PushInt(bytes, 1);
  PushByte(bytes, 0);
  PushInt(bytes, 1);
}

static void SerializeGlobals(u8 **bytes, Module *mod)
{
  u32 i;
  u32 size;

  size = IntSize(mod->num_globals) + 5*mod->num_globals;

  PushByte(bytes, GlobalSection);
  PushInt(bytes, size);
  PushInt(bytes, mod->num_globals);
  for (i = 0; i < mod->num_globals; i++) {
    PushByte(bytes, Int32); /* type */
    PushByte(bytes, 1); /* var */
    PushByte(bytes, I32Const);
    PushByte(bytes, 0);
    PushByte(bytes, EndOp);
  }
}

static void SerializeExports(u8 **bytes, Module *mod)
{
  u32 i, j;
  Export **exports = mod->exports;
  u32 size = mod->size.exports;

  if (VecCount(exports) == 0) return;

  size += IntSize(VecCount(exports));

  PushByte(bytes, ExportSection);
  PushInt(bytes, size);
  PushInt(bytes, VecCount(exports));
  for (i = 0; i < VecCount(exports); i++) {
    Export *export = exports[i];
    u32 namelen = strlen(export->name);
    PushInt(bytes, namelen);
    for (j = 0; j < namelen; j++) {
      PushByte(bytes, export->name[j]);
    }
    PushByte(bytes, 0); /* function export type */
    PushInt(bytes, i);
  }
}

static void SerializeStart(u8 **bytes, Module *mod)
{
  if (mod->start > VecCount(mod->funcs) + VecCount(mod->imports)) return;
  PushByte(bytes, StartSection);
  PushInt(bytes, IntSize(mod->start));
  PushInt(bytes, mod->start);
}

static void SerializeElements(u8 **bytes, Module *mod)
{
  u32 i;
  Func **funcs = mod->funcs;
  u32 size = mod->size.elements;

  size += IntSize(1); /* num segments */
  size += 4; /* segment attrs */
  size += IntSize(VecCount(funcs));

  PushByte(bytes, ElementsSection);
  PushInt(bytes, size);
  PushInt(bytes, 1);

  PushByte(bytes, 0); /* mode 0, active */
  PushByte(bytes, I32Const); /* offset expr */
  PushInt(bytes, 0);
  PushByte(bytes, EndOp);
  PushInt(bytes, VecCount(funcs)); /* vec(funcidx) */
  for (i = 0; i < VecCount(funcs); i++) {
    PushInt(bytes, i);
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

  PushByte(bytes, CodeSection);
  PushInt(bytes, size);
  PushInt(bytes, VecCount(funcs));
  for (i = 0; i < VecCount(funcs); i++) {
    Func *func = funcs[i];
    PushInt(bytes, func->size);

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
