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
    if (IsError(result)) return result;
    item_chunk = NewChunk();
    ChunkWrite(opSet, item_chunk);
    items_chunk = AppendChunk(item_chunk, items_chunk);
    items_chunk = PreservingEnv(result.data.p, items_chunk);
    index_chunk = NewChunk();
    ChunkWrite(opConst, index_chunk);
    ChunkWriteInt(IntVal(i), index_chunk);
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
  assert(num_items > 1);
  for (i = 0; i < num_items; i++) {
    ASTNode *item = node->data.children[num_items - 1 - i];
    Result result = CompileExpr(item, env, imports, false);
    if (IsError(result)) return result;
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

  chunk = NewChunk();
  ChunkWrite(opDrop, chunk);

  result = CompileExpr(node->data.children[1], env, imports, false);
  if (IsError(result)) return result;
  right_chunk = AppendChunk(chunk, result.data.p);

  chunk = NewChunk();
  ChunkWrite(opDup, chunk);
  ChunkWrite(opBranch, chunk);
  ChunkWriteInt(ChunkSize(right_chunk), chunk);

  result = CompileExpr(node->data.children[0], env, imports, false);
  if (IsError(result)) return result;
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

  result = CompileExpr(node->data.children[0], env, imports, returns);
  if (IsError(result)) return result;
  pred_code = result.data.p;

  result = CompileExpr(node->data.children[1], env, imports, returns);
  if (IsError(result)) return result;
  true_code = result.data.p;

  result = CompileExpr(node->data.children[2], env, imports, returns);
  if (IsError(result)) return result;
  false_code = result.data.p;

  if (!returns) {
    ChunkWrite(opBranch, false_code);
    ChunkWriteInt(ChunkSize(true_code), false_code);
  }

  ChunkWrite(opBranch, pred_code);
  ChunkWriteInt(ChunkSize(false_code), pred_code);

  false_code->next = true_code;
  false_code->modifies_env |= true_code->modifies_env;
  false_code->needs_env |= true_code->needs_env;

  chunk = AppendChunk(pred_code, false_code);
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
  result = CompileExpr(value, env, imports, false);
  if (IsError(result)) return result;
  chunk = result.data.p;
  EnvSet(var_node->data.value, assign_num, env);
  WriteDefine(assign_num, chunk);
  return result;
}

static Result CompileStmts(ASTNode *node, Env *env, ImportMap *imports, bool returns)
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
  Chunk *chunk = 0;
  ASTNode *last;
  Result result;
  assert(node->type == doNode);
  num_stmts = VecCount(node->data.children);
  assert(num_stmts > 0);

  last = node->data.children[num_stmts-1];
  assert (last->type != letNode && last->type != defNode);
  result = CompileExpr(last, env, imports, returns);
  if (IsError(result)) return result;
  chunk = result.data.p;

  for (i = 1; i < num_stmts; i++) {
    Chunk *drop_chunk;
    ASTNode *stmt = node->data.children[num_stmts - 1 - i];
    if (stmt->type == letNode || stmt->type == defNode) {
      result = CompileLet(stmt, assign_num++, env, imports);
      if (IsError(result)) return result;
    } else {
      result = CompileExpr(stmt, env, imports, false);
      if (IsError(result)) return result;
      drop_chunk = NewChunk();
      ChunkWrite(opDrop, drop_chunk);
      chunk = AppendChunk(drop_chunk, chunk);
    }

    chunk = PreservingEnv(result.data.p, chunk);
  }

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

static Result CompileDo(ASTNode *node, Env *env, ImportMap *imports, bool returns)
{
  /*
  ; if any assigns:
    <extend_env(num_assigns)>
  <stmts code>
  */
  Chunk *chunk;
  Result result;
  u32 i, j;
  u32 num_assigns = CountAssigns(node);
  assert(node->type == doNode);

  if (num_assigns == 0) return CompileStmts(node, env, imports, returns);

  env = ExtendEnv(num_assigns, env);
  for (i = 0, j = 0; i < VecCount(node->data.children); i++) {
    ASTNode *item = node->data.children[i];
    if (item->type == defNode) {
      ASTNode *var_node = item->data.children[0];
      EnvSet(var_node->data.value, num_assigns - j - 1, env);
      j++;
    }
  }
  chunk = NewChunk();
  WriteExtendEnv(num_assigns, chunk);
  result = CompileStmts(node, env, imports, returns);
  if (IsError(result)) return result;
  chunk = AppendChunk(chunk, result.data.p);
  env = PopEnv(env);
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
  Chunk *chunk;
  ASTNode *params, *body;
  u32 num_params;
  Result result;
  assert(node->type == lambdaNode);
  assert(VecCount(node->data.children) > 1);
  params = node->data.children[0];
  num_params = VecCount(params->data.children);
  body = node->data.children[1];

  chunk = NewChunk();
  if (num_params > 0) {
    u32 i;
    chunk = NewChunk();
    WriteExtendEnv(num_params, chunk);
    env = ExtendEnv(num_params, env);
    for (i = 0; i < num_params; i++) {
      u32 index = num_params - i - 1;
      ASTNode *param = params->data.children[index];
      EnvSet(param->data.value, index, env);
      WriteDefine(index, chunk);
    }
  }

  result = CompileExpr(body, env, imports, true);
  if (IsError(result)) return result;

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

  PrintNode(node);

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
    if (IsError(result)) return result;
    chunk = PreservingEnv(result.data.p, chunk);
  }

  if (primitive_num >= 0) {
    ChunkWrite(opTrap, chunk);
    ChunkWriteInt(primitive_num, chunk);
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

  return Ok(chunk);
}

static i32 FindModuleID(u32 name, Module *modules)
{
  u32 i;
  for (i = 0; i < VecCount(modules); i++) {
    if (modules[i].name == name) return modules[i].id;
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

static Result CompileImports(Module *module, ImportMap *imports)
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

  if (VecCount(module->imports) == 0) return Ok(chunk);

  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(VecCount(module->imports), chunk);

  for (i = 0; i < VecCount(module->imports); i++) {
    assert(HashMapContains(&imports->map, module->imports[i].module));
    import_id = HashMapGet(&imports->map, module->imports[i].module);
    ChunkWrite(opConst, chunk);
    ChunkWriteInt(IntVal(i), chunk);

    ChunkWrite(opGetEnv, chunk);
    chunk = AppendChunk(chunk, CompileCallMod(import_id));
    ChunkWrite(opSetEnv, chunk);

    ChunkWrite(opSet, chunk);
  }
  ChunkWrite(opPair, chunk);
  ChunkWrite(opSetEnv, chunk);
  return Ok(chunk);
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

  u32 i, j;
  Result result;
  Chunk *chunk, *cache;
  u32 num_assigns = CountAssigns(module->ast);
  ImportMap *imports = IndexImports(module, modules);

  result = CompileImports(module, imports);
  if (IsError(result)) return result;
  chunk = result.data.p;
  env = DefineImports(module, env);

  if (num_assigns > 0) {
    env = ExtendEnv(num_assigns, env);
    for (i = 0, j = 0; i < VecCount(module->ast->data.children); i++) {
      ASTNode *item = module->ast->data.children[i];
      if (item->type == defNode) {
        ASTNode *var_node = item->data.children[0];
        EnvSet(var_node->data.value, num_assigns - j - 1, env);
        j++;
      }
    }

    WriteExtendEnv(num_assigns, chunk);
  }
  result = CompileStmts(module->ast, env, imports, false);
  if (IsError(result)) return result;
  chunk = AppendChunk(chunk, result.data.p);
  ChunkWrite(opDrop, chunk);

  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(VecCount(module->exports), chunk);

  for (i = 0; i < VecCount(module->exports); i++) {
    EnvPosition pos = EnvFind(module->exports[i].name, env);
    if (pos.frame == -1) return UndefinedExport(module->filename, &module->exports[i]);
    ChunkWrite(opConst, chunk);
    ChunkWriteInt(IntVal(i), chunk);
    WriteLookup(pos.frame, pos.index, chunk);
    ChunkWrite(opSet, chunk);
    ChunkWrite(opNoop, chunk);
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
  ChunkWrite(opGetMod, chunk);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(module->id), chunk);
  result = CompileModuleBody(module, modules, env);
  if (IsError(result)) return result;
  chunk = AppendChunk(chunk, CompileMakeLambda(result.data.p, false));
  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);
  module->code = chunk;

  return Ok(chunk);
}

Chunk *CompileIntro(u32 num_modules)
{
  /*
  tuple <num_modules>
  setMod
  */
  Chunk *chunk = NewChunk();
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_modules, chunk);
  ChunkWrite(opSetMod, chunk);
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
  switch (node->type) {
  case constNode:   return CompileConst(node, returns);
  case varNode:     return CompileVar(node, env, returns);
  case strNode:     return CompileString(node, returns);
  case tupleNode:   return CompileTuple(node, env, imports, returns);
  case negNode:     return CompileOp(opNeg, node, env, imports, returns);
  case notNode:     return CompileOp(opNot, node, env, imports, returns);
  case lenNode:     return CompileOp(opLen, node, env, imports, returns);
  case accessNode:  return CompileOp(opGet, node, env, imports, returns);
  case compNode:    return CompileOp(opComp, node, env, imports, returns);
  case eqNode:      return CompileOp(opEq, node, env, imports, returns);
  case remNode:     return CompileOp(opRem, node, env, imports, returns);
  case bitandNode:  return CompileOp(opAnd, node, env, imports, returns);
  case mulNode:     return CompileOp(opMul, node, env, imports, returns);
  case addNode:     return CompileOp(opAdd, node, env, imports, returns);
  case subNode:     return CompileOp(opSub, node, env, imports, returns);
  case divNode:     return CompileOp(opDiv, node, env, imports, returns);
  case ltNode:      return CompileOp(opLt, node, env, imports, returns);
  case shiftNode:   return CompileOp(opShift, node, env, imports, returns);
  case gtNode:      return CompileOp(opGt, node, env, imports, returns);
  case joinNode:    return CompileOp(opJoin, node, env, imports, returns);
  case sliceNode:   return CompileOp(opSlice, node, env, imports, returns);
  case bitorNode:   return CompileOp(opOr, node, env, imports, returns);
  case pairNode:    return CompileOp(opPair, node, env, imports, returns);
  case andNode:     return CompileLogic(node, env, imports, returns);
  case orNode:      return CompileLogic(node, env, imports, returns);
  case ifNode:      return CompileIf(node, env, imports, returns);
  case doNode:      return CompileDo(node, env, imports, returns);
  case lambdaNode:  return CompileLambda(node, env, imports, returns);
  case callNode:    return CompileCall(node, env, imports, returns);
  case refNode:     return CompileRef(node, env, imports, returns);
  default:          return UnknownExpr(node);
  }
}
