#include "compile.h"
#include "ops.h"
#include "chunk.h"
#include "mem.h"
#include "primitives.h"
#include "univ/hashmap.h"
#include "univ/symbol.h"
#include <univ/vec.h>
#include <univ/str.h>

typedef struct {
  Module *modules;
  HashMap map;
} ImportMap;

static Result CompileExpr(ASTNode *node, Env *env, ImportMap *imports, bool returns);

static Result UndefinedVariable(ASTNode *node)
{
  Error *error = NewError(0, node->file, node->start, node->end - node->start);
  error->message = StrCat(error->message, "Undefined variable \"");
  error->message = StrCat(error->message, SymbolName(node->data.value));
  error->message = StrCat(error->message, "\"");
  return Err(error);
}

static Result UndefinedExport(char *file, ModuleExport *export)
{
  char *name = SymbolName(export->name);
  Error *error = NewError(0, file, export->pos, strlen(name));
  error->message = StrCat(error->message, "Undefined variable \"");
  error->message = StrCat(error->message, name);
  error->message = StrCat(error->message, "\"");
  return Err(error);
}

static Result UnknownExpr(ASTNode *node)
{
  Error *error = NewError("Unknown expression", node->file, node->start, node->end-node->start);
  return Err(error);
}

static Result Fail(Chunk *chunk, Result result)
{
  FreeChunk(chunk);
  return result;
}

static void WriteReturn(Chunk *chunk)
{
  ChunkWrite(opSwap, chunk);
  ChunkWrite(opGoto, chunk);
}

static void WriteExtendEnv(u32 num_assigns, Chunk *chunk)
{
  chunk->needs_env = true;
  chunk->modifies_env = true;
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_assigns, chunk);
  ChunkWrite(opPair, chunk);
  ChunkWrite(opSetEnv, chunk);
}

static Result CompileConst(ASTNode *node, bool returns)
{
  /*
  const <value>
  */

  Chunk *chunk;
  assert(node->type == constNode);
  chunk = NewChunk();
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(node->data.value, chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static Result CompileString(ASTNode *node, bool returns)
{
  /*
  const <value>
  str
  */

  Chunk *chunk;
  assert(node->type == strNode);
  chunk = NewChunk();
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(node->data.value, chunk);
  ChunkWrite(opStr, chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static void WriteLookup(u32 frame, u32 index, Chunk *chunk)
{
  /*
  getEnv
  ; frame num times:
    tail
  head
  const <index>
  get
  */

  u32 i;
  ChunkWrite(opGetEnv, chunk);
  for (i = 0; i < frame; i++) {
    ChunkWrite(opTail, chunk);
  }
  ChunkWrite(opHead, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(index), chunk);
  ChunkWrite(opGet, chunk);
}

static Result CompileVar(ASTNode *node, Env *env, bool returns)
{
  /*
  lookup <i>  ; environment index
  */

  Chunk *chunk;
  EnvPosition pos;
  assert(node->type == varNode);
  pos = EnvFind(node->data.value, env);
  if (pos.frame < 0) return UndefinedVariable(node);
  chunk = NewChunk();
  chunk->needs_env = true;
  WriteLookup(pos.frame, pos.index, chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static Result CompileTuple(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  tuple <n>     ; tuple length
  ; for each item:
    const <i>   ; item index
    <item code>
    set
  */

  Chunk *chunk, *items_chunk = 0;
  u32 i, num_items;
  assert(node->type == tupleNode);
  num_items = VecCount(node->data.children);
  for (i = 0; i < num_items; i++) {
    Chunk *index_chunk, *item_chunk;
    ASTNode *item = node->data.children[num_items - 1 - i];
    Result result = CompileExpr(item, env, imports, false);
    if (IsError(result)) return Fail(items_chunk, result);
    item_chunk = NewChunk();
    ChunkWrite(opSet, item_chunk);
    items_chunk = AppendChunk(item_chunk, items_chunk);
    items_chunk = PreservingEnv(result.data.p, items_chunk);
    index_chunk = NewChunk();
    ChunkWrite(opConst, index_chunk);
    ChunkWriteInt(IntVal(num_items - 1 - i), index_chunk);
    items_chunk = AppendChunk(index_chunk, items_chunk);
  }
  chunk = NewChunk();
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_items, chunk);
  chunk = AppendChunk(chunk, items_chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static Result CompileOp(OpCode op, ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  <operands code>
  <op>
  */

  Chunk *chunk = 0;
  u32 num_items, i;
  num_items = VecCount(node->data.children);
  assert(num_items > 0);
  for (i = 0; i < num_items; i++) {
    ASTNode *item = node->data.children[num_items - 1 - i];
    Result result = CompileExpr(item, env, imports, false);
    if (IsError(result)) return Fail(chunk, result);
    chunk = PreservingEnv(result.data.p, chunk);
  }
  ChunkWrite(op, chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static Result CompileLogic(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  <left code>
  dup
  ; if orNode:
    not
  branch <after>
  drop
  <right code>
after:
  */

  Chunk *chunk, *left_chunk, *right_chunk;
  Result result;
  assert(node->type == andNode || node->type == orNode);

  result = CompileExpr(node->data.children[1], env, imports, false);
  if (IsError(result)) return result;

  chunk = NewChunk();
  ChunkWrite(opDrop, chunk);
  right_chunk = AppendChunk(chunk, result.data.p);

  chunk = NewChunk();
  ChunkWrite(opDup, chunk);
  ChunkWrite(opBranch, chunk);
  ChunkWriteInt(ChunkSize(right_chunk), chunk);
  right_chunk = AppendChunk(chunk, right_chunk);

  result = CompileExpr(node->data.children[0], env, imports, false);
  if (IsError(result)) return Fail(right_chunk, result);
  left_chunk = result.data.p;

  chunk = PreservingEnv(left_chunk, right_chunk);
  if (returns) WriteReturn(chunk);
  return Ok(chunk);
}

static Result CompileIf(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  <predicate code>
  branch <ifTrue>
  <false code>
  jump <after>
ifTrue:
  <true code>
after:
  */

  Chunk *chunk, *pred_code, *true_code, *false_code;
  Result result;
  assert(node->type == ifNode);

  result = CompileExpr(node->data.children[0], env, imports, false);
  if (IsError(result)) return result;
  pred_code = result.data.p;

  result = CompileExpr(node->data.children[1], env, imports, returns);
  if (IsError(result)) return Fail(pred_code, result);
  true_code = result.data.p;

  result = CompileExpr(node->data.children[2], env, imports, returns);
  if (IsError(result)) return Fail(AppendChunk(pred_code, true_code), result);
  false_code = result.data.p;

  if (!returns) {
    ChunkWrite(opBranch, false_code);
    ChunkWriteInt(ChunkSize(true_code), false_code);
  }

  chunk = NewChunk();
  ChunkWrite(opBranch, chunk);
  ChunkWrite(ChunkSize(false_code), chunk);

  chunk = AppendChunk(chunk, ParallelChunks(false_code, true_code));
  chunk = PreservingEnv(pred_code, chunk);
  return Ok(chunk);
}

static void WriteDefine(u32 index, Chunk *chunk)
{
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opHead, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(index), chunk);
  ChunkWrite(opRot, chunk);
  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);
}

static Result CompileLet(ASTNode *node, u32 assign_num, Env *env, ImportMap *imports)
{
  /*
  <value code>
  define <i>    ; env index
  */

  Result result;
  ASTNode *var_node, *value;
  Chunk *chunk;
  assert(node->type == letNode || node->type == defNode);
  var_node = node->data.children[0];
  value = node->data.children[1];

  chunk = NewChunk();
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opHead, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(assign_num), chunk);

  result = CompileExpr(value, env, imports, false);
  if (IsError(result)) return Fail(chunk, result);
  chunk = AppendChunk(chunk, result.data.p);
  EnvSet(var_node->data.value, assign_num, env);

  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);

  return Ok(chunk);
}

static Result CompileStmts(ASTNode *node, u32 num_assigns, Env *env, ImportMap *imports, bool returns)
{
  /*
  ; for each statement, except the last:
    <stmt code>
    ; if statement isn't a letNode:
      drop
  <last stmt code>
  ; if statement is a letNode:
    const 0
  */

  u32 num_stmts, i, assign_num = 0;
  Chunk **chunks = 0, *chunk;
  ASTNode *last;
  Result result;
  assert(node->type == doNode);
  num_stmts = VecCount(node->data.children);
  assert(num_stmts > 0);

  for (i = 0; i < num_stmts-1; i++) {
    ASTNode *stmt = node->data.children[i];
    if (stmt->type == letNode || stmt->type == defNode) {
      result = CompileLet(stmt, num_assigns - 1 - assign_num, env, imports);
      assign_num++;
    } else {
      result = CompileExpr(stmt, env, imports, false);
    }
    if (IsError(result)) {
      for (i = 0; i < VecCount(chunks); i++) FreeChunk(chunks[i]);
      FreeVec(chunks);
      return result;
    }
    VecPush(chunks, result.data.p);
  }

  last = node->data.children[num_stmts-1];
  assert (last->type != letNode && last->type != defNode);
  result = CompileExpr(last, env, imports, returns);
  if (IsError(result)) {
    for (i = 0; i < VecCount(chunks); i++) FreeChunk(chunks[i]);
    FreeVec(chunks);
    return result;
  }
  chunk = result.data.p;

  for (i = 0; i < VecCount(chunks); i++) {
    Chunk *stmt_chunk = chunks[VecCount(chunks) - i - 1];
    ASTNode *stmt = node->data.children[VecCount(chunks) - i - 1];
    if (stmt->type != letNode && stmt->type != defNode) {
      Chunk *drop_chunk = NewChunk();
      ChunkWrite(opDrop, drop_chunk);
      chunk = AppendChunk(drop_chunk, chunk);
    }
    chunk = PreservingEnv(stmt_chunk, chunk);
  }

  FreeVec(chunks);

  return Ok(chunk);
}

static u32 CountAssigns(ASTNode *node)
{
  u32 num_assigns = 0, i;
  for (i = 0; i < VecCount(node->data.children); i++) {
    ASTNode *item = node->data.children[i];
    if (item->type == letNode || item->type == defNode) num_assigns++;
  }
  return num_assigns;
}

static Env *DefineDefs(ASTNode *node, u32 num_assigns, Env *env)
{
  u32 i, j;
  env = ExtendEnv(num_assigns, env);
  for (i = 0, j = 0; i < VecCount(node->data.children); i++) {
    ASTNode *item = node->data.children[i];
    if (item->type == defNode) {
      ASTNode *var_node = item->data.children[0];
      EnvSet(var_node->data.value, num_assigns - j - 1, env);
      j++;
    } else if (item->type == letNode) {
      j++;
    }
  }
  return env;
}

static Result CompileDo(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  ; if any assigns:
    <extend_env(num_assigns)>
  <stmts code>
  */
  Chunk *chunk;
  Result result;
  u32 num_assigns = CountAssigns(node);
  assert(node->type == doNode);

  if (num_assigns == 0) return CompileStmts(node, 0, env, imports, returns);

  env = DefineDefs(node, num_assigns, env);
  chunk = NewChunk();
  WriteExtendEnv(num_assigns, chunk);
  result = CompileStmts(node, num_assigns, env, imports, returns);
  env = PopEnv(env);
  if (IsError(result)) return Fail(chunk, result);
  chunk = AppendChunk(chunk, result.data.p);
  return Ok(chunk);
}

static Chunk *CompilePos(Chunk *chunk)
{
  Chunk *pos_chunk = NewChunk();
  ChunkWrite(opPos, pos_chunk);
  ChunkWriteInt(ChunkSize(chunk), pos_chunk);
  return AppendChunk(pos_chunk, chunk);
}

static Chunk *CompileMakeLambda(Chunk *body, bool returns)
{
  Chunk *chunk = NewChunk();
  chunk->needs_env = true;
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opPair, chunk);
  if (returns) {
    WriteReturn(chunk);
  } else {
    ChunkWrite(opJump, chunk);
    ChunkWriteInt(ChunkSize(body), chunk);
  }

  chunk = CompilePos(chunk);
  TackOnChunk(chunk, body);

  return chunk;
}

static Result CompileLambdaBody(ASTNode *node, Env *env, ImportMap *imports)
{
  Chunk *chunk = 0;
  ASTNode *params, *body;
  u32 num_params;
  Result result;
  assert(node->type == lambdaNode);
  assert(VecCount(node->data.children) > 1);
  params = node->data.children[0];
  num_params = VecCount(params->data.children);
  body = node->data.children[1];

  if (num_params > 0) {
    u32 i;
    chunk = NewChunk();
    ChunkWrite(opTuple, chunk);
    ChunkWriteInt(num_params, chunk);
    env = ExtendEnv(num_params, env);
    for (i = 0; i < num_params; i++) {
      u32 index = num_params - i - 1;
      ASTNode *param = params->data.children[index];
      EnvSet(param->data.value, index, env);
      ChunkWrite(opConst, chunk);
      ChunkWriteInt(IntVal(index), chunk);
      ChunkWrite(opRot, chunk);
      ChunkWrite(opSet, chunk);
    }
    ChunkWrite(opGetEnv, chunk);
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opPair, chunk);
    ChunkWrite(opSetEnv, chunk);
  }

  result = CompileExpr(body, env, imports, true);
  if (num_params > 0) env = PopEnv(env);
  if (IsError(result)) return Fail(chunk, result);

  chunk = AppendChunk(chunk, result.data.p);
  return Ok(chunk);
}

static Result CompileLambda(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  pos <body>
  getEnv
  pair
  ; if returning after:
    <return>
  ; else:
    jump <after>
body:
  ; if any params:
    <extend env(num_params)>
    ; for reach param:
      define <i>
  <body code>   ; return after
after:
  */

  Chunk *chunk;
  Result result;
  assert(node->type == lambdaNode);
  result = CompileLambdaBody(node, env, imports);
  if (IsError(result)) return result;
  chunk = CompileMakeLambda(result.data.p, returns);

  if (returns) WriteReturn(chunk);

  return Ok(chunk);
}

static void CompileMakeCall(Chunk *chunk)
{
  chunk->modifies_env = true;
  ChunkWrite(opDup, chunk);
  ChunkWrite(opHead, chunk);
  ChunkWrite(opSetEnv, chunk);
  ChunkWrite(opTail, chunk);
  ChunkWrite(opGoto, chunk);
}

static Result CompileCall(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  ; if not returning after:
    pos <after>
  ; for each argument:
    <arg code>
  <func code>
  dup
  head
  setEnv
  tail
  goto
after:
  */

  u32 i, num_items;
  Chunk *chunk;
  Result result;
  i32 primitive_num = -1;
  assert(node->type == callNode);
  num_items = VecCount(node->data.children);
  assert(num_items > 0);

  if (node->data.children[0]->type == varNode) {
    primitive_num = PrimitiveID(node->data.children[0]->data.value);
  }

  if (primitive_num >= 0) {
    chunk = NewChunk();
  } else {
    result = CompileExpr(node->data.children[0], env, imports, false);
    if (IsError(result)) return result;
    chunk = result.data.p;
  }

  for (i = 0; i < num_items - 1; i++) {
    result = CompileExpr(node->data.children[num_items - 1 - i], env, imports, false);
    if (IsError(result)) return Fail(chunk, result);
    chunk = PreservingEnv(result.data.p, chunk);
  }

  if (primitive_num >= 0) {
    ChunkWrite(opTrap, chunk);
    ChunkWriteInt(primitive_num, chunk);
    if (returns) WriteReturn(chunk);
  } else {
    CompileMakeCall(chunk);
    if (!returns) chunk = CompilePos(chunk);
  }

  return Ok(chunk);
}

static Result CompileRef(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  <lookup mod alias>
  const <symbol index>
  get
  */
  ASTNode *mod = node->data.children[0];
  ASTNode *sym = node->data.children[1];
  Result result;
  Chunk *chunk;
  u32 symindex, mod_id;

  if (!HashMapContains(&imports->map, mod->data.value)) {
    return UndefinedVariable(mod);
  }
  mod_id = HashMapGet(&imports->map, mod->data.value);
  for (symindex = 0; symindex < VecCount(imports->modules[mod_id].exports); symindex++) {
    if (imports->modules[mod_id].exports[symindex].name == sym->data.value) break;
  }
  if (symindex == VecCount(imports->modules[mod_id].exports)) {
    return UndefinedVariable(sym);
  }

  result = CompileVar(mod, env, false);
  if (IsError(result)) return result;
  chunk = result.data.p;
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(symindex), chunk);
  ChunkWrite(opGet, chunk);

  if (returns) WriteReturn(chunk);

  return Ok(chunk);
}

static i32 FindModuleID(u32 name, Module *modules)
{
  u32 i;
  for (i = 0; i < VecCount(modules); i++) {
    if (modules[i].name == name) return i;
  }
  return -1;
}

static ImportMap *IndexImports(Module *module, Module *modules)
{
  u32 i;
  ImportMap *imports = malloc(sizeof(ImportMap));
  InitHashMap(&imports->map);
  imports->modules = modules;
  for (i = 0; i < VecCount(module->imports); i++) {
    u32 alias = module->imports[i].alias;
    i32 mod_id = FindModuleID(module->imports[i].module, modules);
    assert(mod_id >= 0);
    if ((u32)mod_id < VecCount(modules)) {
      HashMapSet(&imports->map, alias, mod_id);
    }
  }
  return imports;
}

static Chunk *CompileImports(Module *module, ImportMap *imports)
{
  /*
  getEnv
  tuple <num imports>
  ; for each import:
    const <i>
    getMod
    const <m>
    get
    <call>
    set
  pair
  setEnv
  */
  Chunk *chunk = NewChunk();
  u32 i, import_id;

  if (VecCount(module->imports) == 0) return chunk;

  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(VecCount(module->imports), chunk);

  for (i = 0; i < VecCount(module->imports); i++) {
    assert(HashMapContains(&imports->map, module->imports[i].module));
    import_id = HashMapGet(&imports->map, module->imports[i].module);
    ChunkWrite(opConst, chunk);
    ChunkWriteInt(IntVal(i), chunk);

    ChunkWrite(opGetEnv, chunk);
    chunk = AppendChunk(chunk, CompileCallMod(imports->modules[import_id].id));
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opSetEnv, chunk);

    ChunkWrite(opSet, chunk);
  }
  ChunkWrite(opPair, chunk);
  ChunkWrite(opSetEnv, chunk);
  return chunk;
}

static Env *DefineImports(Module *module, Env *env)
{
  u32 i;
  if (VecCount(module->imports) > 0) {
    env = ExtendEnv(VecCount(module->imports), env);
    for (i = 0; i < VecCount(module->imports); i++) {
      EnvSet(module->imports[i].alias, i, env);
    }
  }
  return env;
}

static Result CompileModuleBody(Module *module, Module *modules, Env *env)
{
  /*
  <imports>

  <body code>
  drop          ; discard body result
  tuple <n>     ; create exports using body env
  ; for each export:
    const <i>
    lookup <f>
    set
  <extend_env(1)>
  dup
  define 0
  pos <cache_body>
  getEnv
  pair
  jump <after_cache>
cache_body:
  lookup 0
  <return>
after_cache:
  getMod
  const <m>
  rot
  set
  <return>
  */

  u32 i;
  Result result;
  Chunk *chunk, *cache;
  u32 num_assigns = CountAssigns(module->ast);
  ImportMap *imports = IndexImports(module, modules);

  chunk = CompileImports(module, imports);
  env = DefineImports(module, env);

  if (num_assigns > 0) {
    env = DefineDefs(module->ast, num_assigns, env);
    WriteExtendEnv(num_assigns, chunk);
  }
  result = CompileStmts(module->ast, num_assigns, env, imports, false);
  if (IsError(result)) {
    if (num_assigns > 0) env = PopEnv(env);
    if (VecCount(module->imports) > 0) env = PopEnv(env);
    DestroyHashMap(&imports->map);
    free(imports);
    return Fail(chunk, result);
  }
  chunk = AppendChunk(chunk, result.data.p);
  ChunkWrite(opDrop, chunk);

  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(VecCount(module->exports), chunk);

  for (i = 0; i < VecCount(module->exports); i++) {
    EnvPosition pos = EnvFind(module->exports[i].name, env);
    if (pos.frame == -1) {
      if (num_assigns > 0) env = PopEnv(env);
      if (VecCount(module->imports) > 0) env = PopEnv(env);
      DestroyHashMap(&imports->map);
      free(imports);
      return Fail(chunk, UndefinedExport(module->filename, &module->exports[i]));
    }
    ChunkWrite(opConst, chunk);
    ChunkWriteInt(IntVal(i), chunk);
    WriteLookup(pos.frame, pos.index, chunk);
    ChunkWrite(opSet, chunk);
  }

  WriteExtendEnv(1, chunk);
  ChunkWrite(opDup, chunk);
  WriteDefine(0, chunk);

  cache = NewChunk();
  ChunkWrite(opGetEnv, cache);
  ChunkWrite(opHead, cache);
  ChunkWrite(opConst, cache);
  ChunkWriteInt(IntVal(0), cache);
  ChunkWrite(opGet, cache);
  WriteReturn(cache);
  cache = CompileMakeLambda(cache, false);

  chunk = AppendChunk(chunk, cache);
  ChunkWrite(opGetMod, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(module->id), chunk);
  ChunkWrite(opRot, chunk);
  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);
  WriteReturn(chunk);

  if (num_assigns > 0) env = PopEnv(env);
  if (VecCount(module->imports) > 0) env = PopEnv(env);
  DestroyHashMap(&imports->map);
  free(imports);

  return Ok(chunk);
}

Result CompileModule(Module *module, Module *modules, Env *env)
{
  /*
  pos <body>
  getEnv
  pair
  jump <after>
body:
  <module body>
after:
  getMod
  const <m>
  rot
  set
  drop
  */

  Chunk *chunk;
  Result result;
  chunk = NewChunk();
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(module->id), chunk);
  result = CompileModuleBody(module, modules, env);
  if (IsError(result)) return Fail(chunk, result);
  chunk = AppendChunk(chunk, CompileMakeLambda(result.data.p, false));
  ChunkWrite(opSet, chunk);
  module->code = chunk;

  return Ok(chunk);
}

Chunk *CompileIntro(u32 num_modules)
{
  /*
  tuple <num_modules>
  */
  Chunk *chunk = NewChunk();
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_modules, chunk);
  return chunk;
}

Chunk *CompileCallMod(u32 id)
{
  /*
  getMod
  const <entry mod>
  get
  <call>
  */
  Chunk *chunk = NewChunk();

  ChunkWrite(opGetMod, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(id), chunk);
  ChunkWrite(opGet, chunk);
  CompileMakeCall(chunk);
  return CompilePos(chunk);
}

static Result CompileExpr(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  Result result;

  switch (node->type) {
  case constNode:   result = CompileConst(node, returns); break;
  case varNode:     result = CompileVar(node, env, returns); break;
  case strNode:     result = CompileString(node, returns); break;
  case tupleNode:   result = CompileTuple(node, env, imports, returns); break;
  case negNode:     result = CompileOp(opNeg, node, env, imports, returns); break;
  case notNode:     result = CompileOp(opNot, node, env, imports, returns); break;
  case lenNode:     result = CompileOp(opLen, node, env, imports, returns); break;
  case accessNode:  result = CompileOp(opGet, node, env, imports, returns); break;
  case compNode:    result = CompileOp(opComp, node, env, imports, returns); break;
  case eqNode:      result = CompileOp(opEq, node, env, imports, returns); break;
  case remNode:     result = CompileOp(opRem, node, env, imports, returns); break;
  case bitandNode:  result = CompileOp(opAnd, node, env, imports, returns); break;
  case mulNode:     result = CompileOp(opMul, node, env, imports, returns); break;
  case addNode:     result = CompileOp(opAdd, node, env, imports, returns); break;
  case subNode:     result = CompileOp(opSub, node, env, imports, returns); break;
  case divNode:     result = CompileOp(opDiv, node, env, imports, returns); break;
  case ltNode:      result = CompileOp(opLt, node, env, imports, returns); break;
  case shiftNode:   result = CompileOp(opShift, node, env, imports, returns); break;
  case gtNode:      result = CompileOp(opGt, node, env, imports, returns); break;
  case joinNode:    result = CompileOp(opJoin, node, env, imports, returns); break;
  case sliceNode:   result = CompileOp(opSlice, node, env, imports, returns); break;
  case bitorNode:   result = CompileOp(opOr, node, env, imports, returns); break;
  case pairNode:    result = CompileOp(opPair, node, env, imports, returns); break;
  case andNode:     result = CompileLogic(node, env, imports, returns); break;
  case orNode:      result = CompileLogic(node, env, imports, returns); break;
  case ifNode:      result = CompileIf(node, env, imports, returns); break;
  case doNode:      result = CompileDo(node, env, imports, returns); break;
  case lambdaNode:  result = CompileLambda(node, env, imports, returns); break;
  case callNode:    result = CompileCall(node, env, imports, returns); break;
  case refNode:     result = CompileRef(node, env, imports, returns); break;
  default:          return UnknownExpr(node);
  }

#if 0
  if (!IsError(result)) {
    PrintNode(node);
    DisassembleChunk(result.data.p);
  }
#endif

  return result;
}
