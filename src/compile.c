#include "compile.h"
#include "mem.h"
#include "ops.h"
#include "primitives.h"
#include "univ/str.h"
#include "univ/symbol.h"

/*
A compiler object holds context for compiling an AST node. `Compile` is a
wrapper around `CompileExpr`, which dispatches to a compile function based on
the node type. Compile functions generally start with "Compile...".

Each compile function takes a node, a compiler object, and whether the node is
in a tail position. Terminal nodes in a tail position should compile code to
return. Each compile function should return a chunk of code on success, or null
on error. Errors should be set on the compiler object using one of the
convenience error functions.

Compile functions may use emitter functions to write bytecode to a chunk.
Emitter functions generally start with "Write...".
*/

void InitCompiler(Compiler *c, Module *modules, HashMap *mod_map)
{
  c->env = 0;
  c->error = 0;
  c->modules = modules;
  c->mod_map = mod_map;
  c->mod_id = 0;
  InitHashMap(&c->alias_map);
}

void DestroyCompiler(Compiler *c)
{
  DestroyHashMap(&c->alias_map);
}

/* Error functions */

static Chunk *UnknownExpr(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Unknown expression \"^\"");
  msg = FormatString(msg, NodeTypeName(node->type));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *UndefinedVariable(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined variable \"^\"");
  msg = FormatString(msg, SymbolName(NodeValue(node)));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *UndefinedTrap(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined trap \"^\"");
  msg = FormatString(msg, SymbolName(NodeValue(node)));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *UndefinedModule(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined module \"^\"");
  msg = FormatString(msg, SymbolName(NodeValue(node)));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *UndefinedImport(u32 mod, ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined import \"^.^\"");
  msg = FormatString(msg, SymbolName(mod));
  msg = FormatString(msg, SymbolName(NodeValue(node)));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *Unimplemented(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Unimplemented expression \"^\"");
  msg = FormatString(msg, NodeTypeName(node->type));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

/* Helper to free a chunk and return null */

static Chunk *Fail(Chunk *chunk)
{
  FreeChunk(chunk);
  return 0;
}

/* Emitter functions */

static void WriteConst(u32 n, Chunk *chunk)
{
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(IntVal(n), chunk);
}

/* Wrap a chunk with a "pos" instruction, pointing to the end of the chunk */
static Chunk *WriteChunkSize(Chunk *chunk)
{
  Chunk *pos_chunk = NewChunk(chunk->src);
  ChunkWrite(opPos, pos_chunk);
  ChunkWriteInt(ChunkSize(chunk), pos_chunk);
  return AppendChunk(pos_chunk, chunk);
}

/* Assumes a return value and result are on the stack */
static void WriteReturn(Chunk *chunk)
{
  /*
  swap
  goto
  */
  ChunkWrite(opSwap, chunk);
  ChunkWrite(opGoto, chunk);
}

/* Looks up a variable given the frame and index number */
static void WriteLookup(u32 frame, u32 index, Chunk *chunk)
{
  /*
  getEnv
  ; for n frames:
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
  WriteConst(index, chunk);
  ChunkWrite(opGet, chunk);
}

/* Extends the runtime environment with a new scope */
static void WriteExtendEnv(u32 num_assigns, Chunk *chunk)
{
  /*
  getEnv
  tuple <num_assigns>
  pair
  setEnv
  */
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_assigns, chunk);
  ChunkWrite(opPair, chunk);
  ChunkWrite(opSetEnv, chunk);
}

/* Extends an environment for a chunk */
static Chunk *WriteScope(u32 num_assigns, Chunk *body)
{
  Chunk *chunk = NewChunk(body->src);
  WriteExtendEnv(num_assigns, chunk);
  return AppendChunk(chunk, body);
}

/* Sets a variable in the environment */
static void WriteDefine(u32 index, Chunk *chunk)
{
  /*
  getEnv
  head
  const <index>
  rot
  set
  drop
  */
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opHead, chunk);
  WriteConst(index, chunk);
  ChunkWrite(opRot, chunk);
  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);
}

/* Sets the value of a module */
static void WriteSetModule(Chunk *chunk, u32 mod_id)
{
  /*
  getMod
  const <mod id>
  rot
  set
  drop
  */
  ChunkWrite(opGetMod, chunk);
  WriteConst(mod_id, chunk);
  ChunkWrite(opRot, chunk);
  ChunkWrite(opSet, chunk);
  ChunkWrite(opDrop, chunk);
}

/* Wraps a lambda body with code to create a lambda */
static Chunk *WriteMakeLambda(Chunk *body, bool returns)
{
  /*
  pos <body>
  getEnv
  pair
  jump <after>
body:
  <lambda body>
after:
  */
  Chunk *chunk = NewChunk(body->src);
  ChunkWrite(opGetEnv, chunk);
  ChunkWrite(opPair, chunk);
  if (returns) {
    WriteReturn(chunk);
  } else {
    ChunkWrite(opJump, chunk);
    ChunkWriteInt(ChunkSize(body), chunk);
  }
  chunk = WriteChunkSize(chunk);
  TackOnChunk(chunk, body);
  return chunk;
}

/* Wraps a chunk in code to perform a function call. Chunk should have set up
arguments and the function on the stack. */
static Chunk *WriteMakeCall(Chunk *chunk, bool returns)
{
  /*
  ; if not tail call, set up call frame:
    link
    pos 0
  <args>
  <func>
  dup
  head
  setEnv
  tail
  goto
  ; if not tail call, clean up call frame:
    swap
    drop
    swap
    unlink
  */
  ChunkWrite(opDup, chunk);
  ChunkWrite(opHead, chunk);
  ChunkWrite(opSetEnv, chunk);
  ChunkWrite(opTail, chunk);
  ChunkWrite(opGoto, chunk);
  if (!returns) {
    Chunk *link = NewChunk(chunk->src);
    ChunkWrite(opLink, link);
    ChunkWrite(opPos, link);
    ChunkWriteInt(0, link);
    chunk = AppendChunk(link, WriteChunkSize(chunk));
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opDrop, chunk);
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opUnlink, chunk);
  }
  return chunk;
}

/* At the end of module functions, they replace the value of the module with a
   new function that returns the memoized exports */
static Chunk *WriteCacheExports(Chunk *chunk, u32 mod_id)
{
  /*
  ; keep copy of exports on the stack
  dup

  ; create lambda, but store the exports as the env
  pos <body>
  swap
  pair
  jump <after>
body:
  ; exports are the env, just return them
  getEnv
  swap
  goto
after:
  ; replace the module function with the cache function
  getMod
  const <mod id>
  rot
  set
  drop
  */
  Chunk *body = NewChunk(chunk->src);
  Chunk *cache = NewChunk(chunk->src);

  ChunkWrite(opGetEnv, body);
  WriteReturn(body);

  ChunkWrite(opSwap, cache);
  ChunkWrite(opPair, cache);
  ChunkWrite(opJump, cache);
  ChunkWriteInt(ChunkSize(body), cache);
  cache = WriteChunkSize(cache);
  cache = AppendChunk(cache, body);
  WriteSetModule(cache, mod_id);

  ChunkWrite(opDup, chunk);
  return AppendChunk(chunk, cache);
}

static Chunk *CompileExpr(ASTNode *node, bool returns, Compiler *c);

static Chunk *CompileConst(ASTNode *node, bool returns, Compiler *c)
{
  /*
  const <value>
  */
  Chunk *chunk;
  chunk = NewChunk(node->start);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(NodeValue(node), chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileVar(ASTNode *node, bool returns, Compiler *c)
{
  /*
  <lookup var>
  */
  Chunk *chunk;
  EnvPosition pos;
  pos = EnvFind(NodeValue(node), c->env);
  if (pos.frame < 0) return UndefinedVariable(node, c);
  chunk = NewChunk(node->start);
  WriteLookup(pos.frame, pos.index, chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileStr(ASTNode *node, bool returns, Compiler *c)
{
  /*
  const sym
  str
  */
  Chunk *chunk = NewChunk(node->start);
  ChunkWrite(opConst, chunk);
  ChunkWriteInt(NodeValue(node), chunk);
  ChunkWrite(opStr, chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileTuple(ASTNode *node, bool returns, Compiler *c)
{
  /*
  tuple <n>
  ; for each item:
    const <i>
    <item>
    set
  */
  Chunk *chunk, *items_chunk = 0;
  u32 i, num_items;

  /* compile from the last element backwards, so we can save/restore env around
     each (if necessary) */
  num_items = NodeCount(node);
  for (i = 0; i < num_items; i++) {
    u32 index = num_items - 1 - i;
    Chunk *item_chunk, *index_chunk, *set_chunk;
    ASTNode *item = NodeChild(node, index);

    item_chunk = CompileExpr(item, false, c);
    if (!item_chunk) return Fail(items_chunk);

    set_chunk = NewChunk(node->start);
    ChunkWrite(opSet, set_chunk);
    items_chunk = AppendChunk(set_chunk, items_chunk);

    items_chunk = PreservingEnv(item_chunk, items_chunk);

    index_chunk = NewChunk(node->start);
    WriteConst(index, index_chunk);
    items_chunk = AppendChunk(index_chunk, items_chunk);
  }
  chunk = NewChunk(node->start);
  ChunkWrite(opTuple, chunk);
  ChunkWriteInt(num_items, chunk);
  chunk = AppendChunk(chunk, items_chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileOp(OpCode op, ASTNode *node, bool returns, Compiler *c)
{
  /*
  ; for each operand:
    <operand>
  <op>
  */
  Chunk *chunk = 0;
  u32 num_items, i;
  num_items = NodeCount(node);
  for (i = 0; i < num_items; i++) {
    ASTNode *item = NodeChild(node, num_items - 1 - i);
    Chunk *result = CompileExpr(item, false, c);
    if (!result) return Fail(chunk);
    chunk = PreservingEnv(result, chunk);
  }
  ChunkWrite(op, chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileLogic(ASTNode *node, bool returns, Compiler *c)
{
  /*
  <left code>
  dup
  ; if andNode:
    not
  branch <after>
  drop
  <right code>
after:
  */

  Chunk *chunk, *left_chunk, *right_chunk;

  right_chunk = CompileExpr(NodeChild(node, 1), false, c);
  if (!right_chunk) return right_chunk;

  chunk = NewChunk(node->start);
  ChunkWrite(opDrop, chunk);
  right_chunk = AppendChunk(chunk, right_chunk);

  chunk = NewChunk(node->start);
  ChunkWrite(opDup, chunk);
  if (node->type == andNode) ChunkWrite(opNot, chunk);
  ChunkWrite(opBranch, chunk);
  ChunkWriteInt(ChunkSize(right_chunk), chunk);
  right_chunk = AppendChunk(chunk, right_chunk);

  left_chunk = CompileExpr(NodeChild(node, 0), false, c);
  if (!left_chunk) return Fail(right_chunk);

  chunk = PreservingEnv(left_chunk, right_chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileIf(ASTNode *node, bool returns, Compiler *c)
{
  /*
  <test>
  branch <false>
  <trueCode>
  ; if not tail call:
    jump <after>
false:
  <falseCode>
after:
  */
  Chunk *chunk, *pred_code, *true_code, *false_code;

  pred_code = CompileExpr(NodeChild(node, 0), false, c);
  if (!pred_code) return pred_code;

  true_code = CompileExpr(NodeChild(node, 1), returns, c);
  if (!true_code) return Fail(pred_code);

  false_code = CompileExpr(NodeChild(node, 2), returns, c);
  if (!false_code) return Fail(AppendChunk(pred_code, true_code));

  if (!returns) {
    ChunkWrite(opJump, false_code);
    ChunkWriteInt(ChunkSize(true_code), false_code);
  }

  chunk = NewChunk(node->start);
  ChunkWrite(opBranch, chunk);
  ChunkWriteInt(ChunkSize(false_code), chunk);

  chunk = AppendChunk(chunk, ParallelChunks(false_code, true_code));
  chunk = PreservingEnv(pred_code, chunk);
  return chunk;
}

static Chunk *CompileDo(ASTNode *node, bool returns, Compiler *c)
{
  u32 i, num_defs;
  Chunk *chunk;
  ASTNode **stmts = 0, **defs = 0;

  /* split up the defs and other statements */
  for (i = 0; i < NodeCount(node); i++) {
    ASTNode *stmt = NodeChild(node, i);
    if (stmt->type == defNode) {
      VecPush(defs, stmt);
    } else {
      VecPush(stmts, stmt);
    }
  }
  num_defs = VecCount(defs);

  chunk = NewChunk(node->start);

  if (num_defs > 0) {
    c->env = ExtendEnv(num_defs, c->env);

    /* define each def ahead of time */
    for (i = 0; i < VecCount(defs); i++) {
      ASTNode *def = defs[i];
      u32 name;
      name = NodeValue(NodeChild(def, 0));
      EnvSet(name, i, c->env);
    }

    /* compile each def, in reverse order to preserve env */
    for (i = 0; i < VecCount(defs); i++) {
      u32 index = num_defs - i - 1;
      Chunk *def_chunk, *set_chunk;
      ASTNode *def = defs[index];
      set_chunk = NewChunk(def->start);
      WriteDefine(index, set_chunk);
      def_chunk = CompileExpr(NodeChild(def, 1), false, c);
      if (!def_chunk) return Fail(chunk);
      def_chunk = PreservingEnv(def_chunk, set_chunk);
      chunk = PreservingEnv(def_chunk, chunk);
    }

    /* prepend the env extension */
    chunk = WriteScope(num_defs, chunk);
  }

  if (VecCount(stmts) == 0) {
    /* no non-def statments; add nil */
    WriteConst(0, chunk);
    if (returns) WriteReturn(chunk);
  } else {
    Chunk *stmts_chunk = NewChunk(node->start);
    /* compile each non-def statement, in reverse */
    for (i = 0; i < VecCount(stmts); i++) {
      u32 index = VecCount(stmts) - i - 1;
      ASTNode *stmt = stmts[index];
      Chunk *stmt_chunk;
      bool is_last = index == VecCount(stmts) - 1;
      stmt_chunk = CompileExpr(stmt, is_last && returns, c);
      if (!stmt_chunk) return Fail(chunk);
      stmts_chunk = PreservingEnv(stmt_chunk, stmts_chunk);
      if (!is_last) ChunkWrite(opDrop, stmts_chunk);
    }
    chunk = AppendChunk(chunk, stmts_chunk);
  }

  if (num_defs > 0) {
    c->env = PopEnv(c->env);
  }

  return chunk;
}

static Chunk *CompileLet(ASTNode *node, bool returns, Compiler *c)
{
  u32 num_assigns = RawInt(NodeValue(NodeChild(node, 0)));
  Chunk *chunk;
  c->env = ExtendEnv(num_assigns, c->env);
  chunk = CompileExpr(NodeChild(node, 1), returns, c);
  if (!chunk) return 0;
  c->env = PopEnv(c->env);
  return WriteScope(num_assigns, chunk);
}

static Chunk *CompileAssign(ASTNode *node, bool returns, Compiler *c)
{
  /*
  assign
    index0
    id0
    val0
    if
      test0
      alt0
      assign
        index1
        id1
        val1
        expr

  Similar to compiling nested if nodes, except each assign statement also sets
  its value in the environment at its index.
  */

  Chunk *val_chunk, *def_chunk, *expr_chunk;
  u32 index = RawInt(NodeValue(NodeChild(node, 0)));
  ASTNode *id = NodeChild(node, 1);
  ASTNode *value = NodeChild(node, 2);
  ASTNode *expr = NodeChild(node, 3);

  val_chunk = CompileExpr(value, false, c);
  if (!val_chunk) return 0;

  def_chunk = NewChunk(node->start);
  WriteDefine(index, def_chunk);
  EnvSet(NodeValue(id), index, c->env);

  val_chunk = PreservingEnv(val_chunk, def_chunk);

  expr_chunk = CompileExpr(expr, returns, c);
  if (!expr_chunk) return Fail(val_chunk);

  return PreservingEnv(val_chunk, expr_chunk);
}

static Chunk *CompileLambdaBody(ASTNode *node, Compiler *c)
{
  /*
  calling convention:
  foo(a, b, c)
  stack:
    <link>
    <call pos>
    <return addr>
    a   ; args, last on top
    b
    c
    <num args>

  ; compare to defined param count
  const <num params plus frame overhead>
  eq
  branch ok
  const :"Wrong number of arguments"
  panic
ok:
  ; define arguments
  tuple <num params>
  ; each arg (reverse order):
    const i
    rot
    set
  ; extend env with arguments
  getEnv
  swap
  pair
  setEnv
  <actual lambda body>
  ; return
  swap
  goto
  */

  Chunk *chunk, *error_chunk, *result;
  ASTNode *params = NodeChild(node, 0);
  ASTNode *body = NodeChild(node, 1);
  u32 num_params = NodeCount(params);

  error_chunk = NewChunk(node->start);
  WriteConst(Symbol("Wrong number of arguments"), error_chunk);
  ChunkWrite(opPanic, error_chunk);

  /* check that the number of params matches */
  chunk = NewChunk(node->start);
  WriteConst(num_params, chunk);
  ChunkWrite(opEq, chunk);
  ChunkWrite(opBranch, chunk);
  ChunkWriteInt(ChunkSize(error_chunk), chunk);
  chunk = AppendChunk(chunk, error_chunk);

  /* define arguments */
  if (num_params > 0) {
    u32 i;
    ChunkWrite(opTuple, chunk);
    ChunkWriteInt(num_params, chunk);
    c->env = ExtendEnv(num_params, c->env);
    for (i = 0; i < num_params; i++) {
      u32 index = num_params - i - 1;
      ASTNode *param = NodeChild(params, index);
      EnvSet(NodeValue(param), index, c->env);
      WriteConst(index, chunk);
      ChunkWrite(opRot, chunk);
      ChunkWrite(opSet, chunk);
    }
    ChunkWrite(opGetEnv, chunk);
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opPair, chunk);
    ChunkWrite(opSetEnv, chunk);
  }

  result = CompileExpr(body, true, c);
  if (num_params > 0) c->env = PopEnv(c->env);
  if (!result) return Fail(chunk);

  chunk = AppendChunk(chunk, result);
  return chunk;
}

static Chunk *CompileLambda(ASTNode *node, bool returns, Compiler *c)
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
  <lambda body>
after:
  */

  Chunk *chunk, *body;
  body = CompileLambdaBody(node, c);
  if (!body) return body;
  chunk = WriteMakeLambda(body, returns);
  return chunk;
}

static Chunk *CompileArgs(ASTNode *node, Compiler *c)
{
  u32 i, num_args = NodeCount(node);
  Chunk *chunk = NewChunk(node->start);

  for (i = 0; i < num_args; i++) {
    u32 index = NodeCount(node) - 1 - i;
    ASTNode *arg = NodeChild(node, index);
    Chunk *arg_chunk = CompileExpr(arg, false, c);
    if (!arg_chunk) return Fail(chunk);
    chunk = PreservingEnv(arg_chunk, chunk);
  }

  return chunk;
}

static Chunk *CompileTrap(ASTNode *node, bool returns, Compiler *c)
{
  i32 primitive_num;
  Chunk *chunk;
  ASTNode *id = NodeChild(node, 0);
  ASTNode *args = NodeChild(node, 1);
  primitive_num = PrimitiveID(NodeValue(id));
  if (primitive_num < 0) return UndefinedTrap(id, c);
  chunk = CompileArgs(args, c);
  if (!chunk) return 0;
  ChunkWrite(opTrap, chunk);
  ChunkWriteInt(primitive_num, chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileCall(ASTNode *node, bool returns, Compiler *c)
{
  /*
  calling convention:
  foo(a, b, c)
  stack:
    <link>
    <call pos>
    <return addr>
    a   ; args, last on top
    b
    c
    const <num args>

  ; if not a tail-call, set up a frame:
    link
    pos 0
    pos <after>
  ; each argument:
    <arg code>
  const <num args>
  <func code>
  dup
  head
  setEnv
  tail
  goto
after:
  ; if not a tail-call, clean up frame:
    swap
    drop
    swap
    unlink
  */

  ASTNode *args = NodeChild(node, 1);
  Chunk *chunk, *args_chunk;

  chunk = CompileExpr(NodeChild(node, 0), false, c);
  if (!chunk) return 0;

  args_chunk = CompileArgs(NodeChild(node, 1), c);
  if (!args_chunk) return Fail(chunk);
  WriteConst(NodeCount(args), args_chunk);

  chunk = PreservingEnv(args_chunk, chunk);
  chunk = WriteMakeCall(chunk, returns);

  return chunk;
}

static Chunk *CompileRef(ASTNode *node, bool returns, Compiler *c)
{
  /*
  Compiler has a map of alias -> module for imports
  Get the fn index from the imported module's AST

  getMod
  const <mod index>
  get
  const <fn index>
  get
  */
  ASTNode *alias = NodeChild(node, 0);
  ASTNode *sym = NodeChild(node, 1);
  Module *mod;
  ASTNode *mod_exports;
  Chunk *chunk;
  u32 sym_index, mod_index;

  if (!HashMapContains(&c->alias_map, NodeValue(alias))) {
    return UndefinedVariable(alias, c);
  }
  mod_index = HashMapGet(&c->alias_map, NodeValue(alias));
  mod = &c->modules[mod_index];
  mod_exports = NodeChild(mod->ast, 2);

  for (sym_index = 0; sym_index < NodeCount(mod_exports); sym_index++) {
    ASTNode *export = NodeChild(mod_exports, sym_index);
    if (NodeValue(export) == NodeValue(sym)) break;
  }
  if (sym_index == NodeCount(mod_exports)) {
    return UndefinedVariable(sym, c);
  }

  chunk = NewChunk(node->start);
  ChunkWrite(opGetMod, chunk);
  WriteConst(mod->id, chunk);
  ChunkWrite(opGet, chunk);
  WriteConst(sym_index, chunk);
  ChunkWrite(opGet, chunk);
  if (returns) WriteReturn(chunk);
  return chunk;
}

static Chunk *CompileImport(ASTNode *node, u32 *next_index, Compiler *c)
{
  /*
  ; env frame is on the stack
  ; call module
  link
  pos 0
  pos <after>
  getMod
  const <import mod id>
  get
  dup
  head
  setEnv
  tail
  goto
after:
  swap
  drop
  swap
  unlink
  ; {imports} {exports}
  ; assign each direct import:
    dup
    const <fn index>
    get
    rot
    const <next index>
    rot
    set
    swap
  drop
  */
  Chunk *chunk;
  Module *import_mod;
  u32 i, import_index, name, alias;
  ASTNode *fns;
  name = NodeValue(NodeChild(node, 0));
  alias = NodeValue(NodeChild(node, 1));
  fns = NodeChild(node, 2);

  if (!HashMapContains(c->mod_map, name)) {
    return UndefinedModule(NodeChild(node, 0), c);
  }
  import_index = HashMapGet(c->mod_map, name);
  import_mod = &c->modules[import_index];

  chunk = NewChunk(node->start);
  ChunkWrite(opGetMod, chunk);
  WriteConst(import_mod->id, chunk);
  ChunkWrite(opGet, chunk);
  chunk = WriteMakeCall(chunk, false);

  for (i = 0; i < NodeCount(fns); i++) {
    ASTNode *fn_node = NodeChild(fns, i);
    u32 fn_name, fn_index;
    fn_name = NodeValue(fn_node);
    if (!HashMapContains(&import_mod->exports, fn_name)) {
      FreeChunk(chunk);
      return UndefinedImport(name, fn_node, c);
    }
    fn_index = HashMapGet(&import_mod->exports, fn_name);

    ChunkWrite(opDup, chunk);
    WriteConst(fn_index, chunk);
    ChunkWrite(opGet, chunk);
    ChunkWrite(opRot, chunk);
    WriteConst(*next_index, chunk);
    ChunkWrite(opRot, chunk);
    ChunkWrite(opSet, chunk);
    ChunkWrite(opSwap, chunk);
    EnvSet(fn_name, *next_index, c->env);
    (*next_index)++;
  }

  ChunkWrite(opDrop, chunk);
  HashMapSet(&c->alias_map, alias, import_index);

  return chunk;
}

static u32 CountImports(ASTNode *node)
{
  u32 count = 0, i;
  for (i = 0; i < NodeCount(node); i++) {
    ASTNode *import = NodeChild(node, i);
    ASTNode *direct_imports = NodeChild(import, 2);
    count += NodeCount(direct_imports);
  }
  return count;
}

static Chunk *CompileImports(ASTNode *node, u32 num_imports, Compiler *c)
{
  /*
  ; set up frame for imports
  tuple <num direct imports>
  ; each import module:
    <import module>
  ; extend env
  getEnv
  swap
  pair
  setEnv
  */

  u32 i;
  u32 next_index = 0;
  Chunk *chunk = NewChunk(node->start);

  if (num_imports > 0) {
    ChunkWrite(opTuple, chunk);
    ChunkWriteInt(num_imports, chunk);
  }

  for (i = 0; i < NodeCount(node); i++) {
    ASTNode *import = NodeChild(node, i);
    Chunk *import_chunk = CompileImport(import, &next_index, c);
    if (!import_chunk) return Fail(chunk);
    chunk = AppendChunk(chunk, import_chunk);
  }

  if (num_imports > 0) {
    ChunkWrite(opGetEnv, chunk);
    ChunkWrite(opSwap, chunk);
    ChunkWrite(opPair, chunk);
    ChunkWrite(opSetEnv, chunk);
  }
  return chunk;
}

static Chunk *CompileModule(ASTNode *node, bool returns, Compiler *c)
{
  /*
  TODO:
  before evaluating imports in a module body, replace the module with the empty
  memoized export tuple. any references to this module's exports in its
  dependencies will then get a reference to that.

  won't that copy a nil value into the scope of the importing module?
  */

  /*
  ; if main function:
  <imports>
  <body>

  ; if named function:
  ; create module function
  pos <body>
  getEnv
  pair
  jump <after>
body:
  ; module body
  <imports>
  <body>
  <cache exports>
  ; return
  swap
  goto
after:
  ; set the module
  getMod
  const <mod id>
  rot
  set
  drop
  */

  Chunk *chunk, *imports = 0;
  ASTNode *imports_node = NodeChild(node, 1);
  u32 mod_name = NodeValue(NodeChild(node, 0));

  u32 num_imports = CountImports(imports_node);
  if (NodeCount(imports_node) > 0) {
    if (num_imports > 0) c->env = ExtendEnv(num_imports, c->env);
    imports = CompileImports(imports_node, num_imports, c);
    if (!imports) return 0;
  }

  chunk = CompileExpr(NodeChild(node, 3), false, c);
  if (!chunk) return Fail(imports);
  if (NodeCount(imports_node) > 0) chunk = AppendChunk(imports, chunk);

  if (mod_name) {
    chunk = WriteCacheExports(chunk, c->mod_id);
    WriteReturn(chunk);
    chunk = WriteMakeLambda(chunk, false);
    WriteSetModule(chunk, c->mod_id);
  }

  if (num_imports > 0) c->env = PopEnv(c->env);
  return chunk;
}

static Chunk *CompileExpr(ASTNode *node, bool returns, Compiler *c)
{
  switch (node->type) {
  case idNode:      return CompileVar(node, returns, c);
  case constNode:   return CompileConst(node, returns, c);
  case strNode:     return CompileStr(node, returns, c);
  case tupleNode:   return CompileTuple(node, returns, c);
  case lambdaNode:  return CompileLambda(node, returns, c);
  case panicNode:   return CompileOp(opPanic, node, false, c);
  case negNode:     return CompileOp(opNeg, node, returns, c);
  case notNode:     return CompileOp(opNot, node, returns, c);
  case headNode:    return CompileOp(opHead, node, returns, c);
  case tailNode:    return CompileOp(opTail, node, returns, c);
  case lenNode:     return CompileOp(opLen, node, returns, c);
  case compNode:    return CompileOp(opComp, node, returns, c);
  case eqNode:      return CompileOp(opEq, node, returns, c);
  case remNode:     return CompileOp(opRem, node, returns, c);
  case bitandNode:  return CompileOp(opAnd, node, returns, c);
  case mulNode:     return CompileOp(opMul, node, returns, c);
  case addNode:     return CompileOp(opAdd, node, returns, c);
  case subNode:     return CompileOp(opSub, node, returns, c);
  case divNode:     return CompileOp(opDiv, node, returns, c);
  case ltNode:      return CompileOp(opLt, node, returns, c);
  case shiftNode:   return CompileOp(opShift, node, returns, c);
  case gtNode:      return CompileOp(opGt, node, returns, c);
  case joinNode:    return CompileOp(opJoin, node, returns, c);
  case sliceNode:   return CompileOp(opSlice, node, returns, c);
  case bitorNode:   return CompileOp(opOr, node, returns, c);
  case pairNode:    return CompileOp(opPair, node, returns, c);
  case andNode:     return CompileLogic(node, returns, c);
  case orNode:      return CompileLogic(node, returns, c);
  case accessNode:  return CompileOp(opGet, node, returns, c);
  case callNode:    return CompileCall(node, returns, c);
  case trapNode:    return CompileTrap(node, returns, c);
  case refNode:     return CompileRef(node, returns, c);
  case ifNode:      return CompileIf(node, returns, c);
  case letNode:     return CompileLet(node, returns, c);
  case assignNode:  return CompileAssign(node, returns, c);
  case doNode:      return CompileDo(node, returns, c);
  case moduleNode:  return CompileModule(node, returns, c);
  default:          return UnknownExpr(node, c);
  }
}

Chunk *Compile(ASTNode *node, Compiler *c)
{
  return CompileExpr(node, false, c);
}
