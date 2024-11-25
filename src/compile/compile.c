#include "compile/compile.h"
#include "runtime/mem.h"
#include "runtime/ops.h"
#include "runtime/primitives.h"
#include "runtime/symbol.h"
#include "univ/str.h"

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
  c->current_mod = 0;
  InitHashMap(&c->alias_map);
  InitHashMap(&c->host_imports);
}

void DestroyCompiler(Compiler *c)
{
  DestroyHashMap(&c->alias_map);
  DestroyHashMap(&c->host_imports);
}

/* Error functions */

static Chunk *UndefinedVariable(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined variable \"^\"");
  msg = FormatString(msg, SymbolName(NodeValue(node)));
  c->error = NewError(msg, 0, node->start, node->end - node->start);
  return 0;
}

static Chunk *UndefinedTrap(ASTNode *node, Compiler *c)
{
  char *msg = NewString("Undefined host function \"^\"");
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

/* Helper to free a chunk and return null */

static Chunk *CompileFail(Chunk *chunk)
{
  FreeChunk(chunk);
  return 0;
}

/* Emitter functions */

static void EmitConst(u32 n, Chunk *chunk)
{
  Emit(opConst, chunk);
  EmitInt(IntVal(n), chunk);
}

/* Wrap a chunk with a "pos" instruction, pointing to the end of the chunk */
static Chunk *EmitChunkSize(Chunk *chunk)
{
  Chunk *pos_chunk = NewChunk(chunk->src);
  Emit(opPos, pos_chunk);
  EmitInt(ChunkSize(chunk), pos_chunk);
  return AppendChunk(pos_chunk, chunk);
}

/* Assumes a return value and result are on the stack */
static void EmitReturn(Chunk *chunk)
{
  /*
  swap
  goto
  */
  Emit(opSwap, chunk);
  Emit(opGoto, chunk);
}

static void EmitGetEnv(Chunk *chunk)
{
  Emit(opPush, chunk);
  EmitInt(regEnv, chunk);
  chunk->needs_env = true;
}

static void EmitSetEnv(Chunk *chunk)
{
  Emit(opPull, chunk);
  EmitInt(regEnv, chunk);
  chunk->modifies_env = true;
}

static void EmitGetMod(Chunk *chunk)
{
  Emit(opPush, chunk);
  EmitInt(regMod, chunk);
}

static void WriteCheckMod(Chunk *chunk)
{
  /*
  dup
  br :ok
  panic "Module unavailable"
ok:
  */
  Chunk *error = NewChunk(chunk->src);
  EmitConst(Symbol("Module unavailable"), error);
  Emit(opPanic, error);
  Emit(opDup, chunk);
  Emit(opBranch, chunk);
  EmitInt(ChunkSize(error), chunk);
  chunk = AppendChunk(chunk, error);
}

static void EmitSetMod(Chunk *chunk)
{
  Emit(opPull, chunk);
  EmitInt(regMod, chunk);
}

/* Extends an environment for a chunk */
static Chunk *EmitScope(u32 num_assigns, u32 pos, Chunk *body)
{
  /*
  getEnv
  tuple <num_assigns>
  pair
  setEnv
  */
  Chunk *chunk = NewChunk(pos);
  EmitGetEnv(chunk);
  Emit(opTuple, chunk);
  EmitInt(num_assigns, chunk);
  Emit(opPair, chunk);
  EmitSetEnv(chunk);
  return AppendChunk(chunk, body);
}

/* Looks up a variable given the frame and index number */
static void EmitLookup(u32 index, Chunk *chunk)
{
  /*
  getEnv
  lookup n
  */
  EmitGetEnv(chunk);
  Emit(opLookup, chunk);
  EmitInt(index, chunk);
}

/* Sets a variable in the environment */
static void EmitDefine(u32 index, Chunk *chunk)
{
  /*
  getEnv
  swap
  define i
  */
  EmitGetEnv(chunk);
  Emit(opSwap, chunk);
  Emit(opDefine, chunk);
  EmitInt(index, chunk);
}

/* Sets the value of a module */
static void EmitSetModule(Chunk *chunk, u32 mod_id)
{
  /*
  getMod
  const <mod id>
  rot
  set
  drop
  */
  EmitGetMod(chunk);
  EmitConst(mod_id - 1, chunk);
  Emit(opRot, chunk);
  Emit(opSet, chunk);
  Emit(opDrop, chunk);
}

/* Wraps a lambda body with code to create a lambda */
static Chunk *EmitMakeLambda(Chunk *body, bool returns)
{
  /*
  tuple 3
  const 0
  const :fn
  set
  const 1
  getEnv
  set
  const 2
  pos <body>
  set
  jump <after>
body:
  <lambda body>
after:
  */
  Chunk *chunk = NewChunk(body->src);
  Chunk *after = NewChunk(body->src);

  Emit(opSet, after);
  if (returns) {
    EmitReturn(after);
  } else {
    Emit(opJump, after);
    EmitInt(ChunkSize(body), after);
  }
  after = EmitChunkSize(after);

  Emit(opTuple, chunk);
  EmitInt(3, chunk);
  EmitConst(0, chunk);
  EmitConst(Symbol("fn"), chunk);
  Emit(opSet, chunk);
  EmitConst(1, chunk);
  EmitGetEnv(chunk);
  Emit(opSet, chunk);
  EmitConst(2, chunk);
  chunk = AppendChunk(chunk, after);
  TackOnChunk(chunk, body);
  return chunk;
}

/* Wraps a chunk in code to perform a function call. Chunk should have set up
arguments and the function on the stack. */
static Chunk *EmitMakeCall(Chunk *chunk, bool returns)
{
  /*
  ; if not tail call, set up call frame:
    link
    pos 0
  <args>
  <func>
  dup
  const 1
  get
  setEnv
  const 2
  get
  goto
  ; if not tail call, clean up call frame:
    swap
    drop
    swap
    unlink
  */

  Emit(opDup, chunk);
  EmitConst(1, chunk);
  Emit(opGet, chunk);
  EmitSetEnv(chunk);
  EmitConst(2, chunk);
  Emit(opGet, chunk);
  Emit(opGoto, chunk);
  if (!returns) {
    Chunk *link = NewChunk(chunk->src);
    Emit(opLink, link);
    Emit(opPos, link);
    EmitInt(0, link);
    chunk = AppendChunk(link, EmitChunkSize(chunk));
    Emit(opSwap, chunk);
    Emit(opDrop, chunk);
    Emit(opSwap, chunk);
    Emit(opUnlink, chunk);
  }
  return chunk;
}

static Chunk *CompileExpr(ASTNode *node, bool returns, Compiler *c);
static Chunk *CompileRef(
  ASTNode *alias, ASTNode *sym, bool returns, Compiler *c);

static Chunk *CompileConst(ASTNode *node, bool returns, Compiler *c)
{
  /*
  const <value>
  */
  Chunk *chunk;
  chunk = NewChunk(node->start);
  Emit(opConst, chunk);
  EmitInt(NodeValue(node), chunk);
  if (returns) EmitReturn(chunk);
  return chunk;
}

static ASTNode *FindImportedAlias(u32 fn_name, ASTNode *imports)
{
  u32 i;
  for (i = 0; i < NodeCount(imports); i++) {
    ASTNode *import = NodeChild(imports, i);
    ASTNode *fns = NodeChild(import, 2);
    u32 j;
    for (j = 0; j < NodeCount(fns); j++) {
      ASTNode *fn = NodeChild(fns, j);
      if (NodeValue(fn) == fn_name) {
        return NodeChild(import, 1);
      }
    }
  }
  return 0;
}

static Chunk *CompileVar(ASTNode *node, bool returns, Compiler *c)
{
  /*
  <lookup var>
  */
  Chunk *chunk;
  u32 name = NodeValue(node);
  i32 pos = EnvFind(name, c->env);

  /* variable exists in scope */
  if (pos >= 0) {
    chunk = NewChunk(node->start);
    EmitLookup(pos, chunk);
    if (returns) EmitReturn(chunk);
    return chunk;
  }

  if (pos < 0) {
    ASTNode *imports = ModuleImports(&c->modules[c->current_mod]);
    ASTNode *alias = FindImportedAlias(name, imports);
    if (alias) {
      return CompileRef(alias, node, returns, c);
    }
  }

  return UndefinedVariable(node, c);
}

static Chunk *CompileStr(ASTNode *node, bool returns, Compiler *c)
{
  /*
  const sym
  str
  */
  Chunk *chunk = NewChunk(node->start);
  Emit(opConst, chunk);
  EmitInt(NodeValue(node), chunk);
  Emit(opStr, chunk);
  if (returns) EmitReturn(chunk);
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
    if (!item_chunk) return CompileFail(items_chunk);

    set_chunk = NewChunk(node->start);
    Emit(opSet, set_chunk);
    items_chunk = AppendChunk(set_chunk, items_chunk);

    items_chunk = PreservingEnv(item_chunk, items_chunk);

    index_chunk = NewChunk(node->start);
    EmitConst(index, index_chunk);
    items_chunk = AppendChunk(index_chunk, items_chunk);
  }
  chunk = NewChunk(node->start);
  Emit(opTuple, chunk);
  EmitInt(num_items, chunk);
  chunk = AppendChunk(chunk, items_chunk);
  if (returns) EmitReturn(chunk);
  return chunk;
}

static Chunk *CompileOp(OpCode op, ASTNode *node, bool returns, Compiler *c)
{
  /*
  ; for each operand:
    <operand>
  <op>
  */
  Chunk *chunk = NewChunk(node->start);
  u32 num_items, i;
  num_items = NodeCount(node);
  for (i = 0; i < num_items; i++) {
    ASTNode *item = NodeChild(node, num_items - 1 - i);
    Chunk *result = CompileExpr(item, false, c);
    if (!result) return CompileFail(chunk);
    chunk = PreservingEnv(result, chunk);
  }
  Emit(op, chunk);
  if (returns) EmitReturn(chunk);
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
  Emit(opDrop, chunk);
  right_chunk = AppendChunk(chunk, right_chunk);

  chunk = NewChunk(node->start);
  Emit(opDup, chunk);
  if (node->type == andNode) Emit(opNot, chunk);
  Emit(opBranch, chunk);
  EmitInt(ChunkSize(right_chunk), chunk);
  right_chunk = AppendChunk(chunk, right_chunk);

  left_chunk = CompileExpr(NodeChild(node, 0), false, c);
  if (!left_chunk) return CompileFail(right_chunk);

  chunk = PreservingEnv(left_chunk, right_chunk);
  if (returns) EmitReturn(chunk);
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
  if (!true_code) return CompileFail(pred_code);

  false_code = CompileExpr(NodeChild(node, 2), returns, c);
  if (!false_code) return CompileFail(AppendChunk(pred_code, true_code));

  if (!returns) {
    Emit(opJump, false_code);
    EmitInt(ChunkSize(true_code), false_code);
  }

  chunk = NewChunk(node->start);
  Emit(opBranch, chunk);
  EmitInt(ChunkSize(false_code), chunk);

  chunk = AppendChunk(chunk, ParallelChunks(false_code, true_code));
  chunk = PreservingEnv(pred_code, chunk);
  return chunk;
}

static Chunk *CompileDo(ASTNode *node, bool returns, Compiler *c)
{
  u32 i, num_defs;
  Chunk *chunk;
  ASTNode **stmts = 0, **defs = 0; /* vec */

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
      EmitDefine(index, set_chunk);
      def_chunk = CompileExpr(NodeChild(def, 1), false, c);
      if (!def_chunk) {
        FreeVec(defs);
        FreeVec(stmts);
        return CompileFail(chunk);
      }
      def_chunk = PreservingEnv(def_chunk, set_chunk);
      chunk = PreservingEnv(def_chunk, chunk);
    }

    /* prepend the env extension */
    chunk = EmitScope(num_defs, node->start, chunk);
  }

  if (VecCount(stmts) == 0) {
    /* no non-def statments; add nil */
    EmitConst(0, chunk);
    if (returns) EmitReturn(chunk);
  } else {
    Chunk *stmts_chunk = NewChunk(node->start);
    /* compile each non-def statement, in reverse */
    for (i = 0; i < VecCount(stmts); i++) {
      u32 index = VecCount(stmts) - i - 1;
      ASTNode *stmt = stmts[index];
      Chunk *stmt_chunk;
      bool is_last = index == VecCount(stmts) - 1;
      stmt_chunk = CompileExpr(stmt, is_last && returns, c);
      if (!stmt_chunk) {
        FreeVec(defs);
        FreeVec(stmts);
        return CompileFail(chunk);
      }
      if (!is_last) stmts_chunk = PrependChunk(opDrop, stmts_chunk);
      stmts_chunk = PreservingEnv(stmt_chunk, stmts_chunk);
    }
    chunk = AppendChunk(chunk, stmts_chunk);
  }

  if (num_defs > 0) {
    c->env = PopEnv(c->env);
  }

  FreeVec(defs);
  FreeVec(stmts);
  return chunk;
}

static Chunk *CompileLet(ASTNode *node, bool returns, Compiler *c)
{
  u32 num_assigns = GetNodeAttr(node, "count");
  Chunk *chunk;
  c->env = ExtendEnv(num_assigns, c->env);
  chunk = CompileExpr(NodeChild(node, 0), returns, c);
  if (!chunk) return 0;
  c->env = PopEnv(c->env);
  return EmitScope(num_assigns, node->start, chunk);
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
  u32 index = GetNodeAttr(node, "index");
  ASTNode *id = NodeChild(node, 0);
  ASTNode *value = NodeChild(node, 1);
  ASTNode *expr = NodeChild(node, 2);

  val_chunk = CompileExpr(value, false, c);
  if (!val_chunk) return 0;

  def_chunk = NewChunk(node->start);
  EmitDefine(index, def_chunk);
  EnvSet(NodeValue(id), index, c->env);

  val_chunk = PreservingEnv(val_chunk, def_chunk);

  expr_chunk = CompileExpr(expr, returns, c);
  if (!expr_chunk) return CompileFail(val_chunk);

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
  const <num params>
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
  EmitConst(Symbol("Wrong number of arguments"), error_chunk);
  Emit(opPanic, error_chunk);

  /* check that the number of params matches */
  chunk = NewChunk(node->start);
  EmitConst(num_params, chunk);
  Emit(opEq, chunk);
  Emit(opBranch, chunk);
  EmitInt(ChunkSize(error_chunk), chunk);
  chunk = AppendChunk(chunk, error_chunk);

  /* define arguments */
  if (num_params > 0) {
    u32 i;
    Emit(opTuple, chunk);
    EmitInt(num_params, chunk);
    c->env = ExtendEnv(num_params, c->env);
    for (i = 0; i < num_params; i++) {
      u32 index = num_params - i - 1;
      ASTNode *param = NodeChild(params, index);
      EnvSet(NodeValue(param), index, c->env);
      EmitConst(index, chunk);
      Emit(opRot, chunk);
      Emit(opSet, chunk);
    }
    EmitGetEnv(chunk);
    Emit(opSwap, chunk);
    Emit(opPair, chunk);
    EmitSetEnv(chunk);
  }

  result = CompileExpr(body, true, c);
  if (!result) return CompileFail(chunk);

  if (num_params > 0) c->env = PopEnv(c->env);

  chunk = AppendChunk(chunk, result);
  return chunk;
}

static Chunk *CompileLambda(ASTNode *node, bool returns, Compiler *c)
{
  /*
  tuple 3
  const 0
  const :fn
  set
  const 1
  pos <body>
  set
  const 2
  getEnv
  set
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
  chunk = EmitMakeLambda(body, returns);
  return chunk;
}

static Chunk *CompileArgs(ASTNode *node, Chunk *chunk, Compiler *c)
{
  u32 i, num_args = NodeCount(node);

  for (i = 0; i < num_args; i++) {
    u32 index = NodeCount(node) - 1 - i;
    ASTNode *arg = NodeChild(node, index);
    Chunk *arg_chunk = CompileExpr(arg, false, c);
    if (!arg_chunk) return CompileFail(chunk);
    chunk = PreservingEnv(arg_chunk, chunk);
  }

  return chunk;
}

static bool IsHostRef(ASTNode *node, Compiler *c)
{
  return node->type == refNode &&
         NodeValue(NodeChild(node, 0)) == Symbol("Host");
}

static bool IsImportedHostFn(ASTNode *node, Compiler *c)
{
  return node->type == idNode &&
         HashMapContains(&c->host_imports, NodeValue(node));
}

static Chunk *CompileTrapCall(ASTNode *id, ASTNode *args, bool returns, Compiler *c)
{
  i32 primitive_num;
  Chunk *chunk;
  primitive_num = PrimitiveID(NodeValue(id));
  if (primitive_num < 0) return UndefinedTrap(id, c);

  chunk = NewChunk(id->start);
  Emit(opTrap, chunk);
  EmitInt(primitive_num, chunk);
  if (returns) EmitReturn(chunk);

  chunk = CompileArgs(args, chunk, c);
  if (!chunk) return 0;
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
  Chunk *chunk, *arity;
  u32 src = node->start;

  if (IsHostRef(NodeChild(node, 0), c)) {
    ASTNode *fn = NodeChild(NodeChild(node, 0), 1);
    return CompileTrapCall(fn, args, returns, c);
  }
  if (IsImportedHostFn(NodeChild(node, 0), c)) {
    ASTNode *fn = NodeChild(node, 0);
    return CompileTrapCall(fn, args, returns, c);
  }

  chunk = CompileExpr(NodeChild(node, 0), false, c);
  if (!chunk) return 0;

  arity = NewChunk(chunk->src);
  EmitConst(NodeCount(args), arity);
  chunk = AppendChunk(arity, chunk);

  chunk = CompileArgs(NodeChild(node, 1), chunk, c);
  if (!chunk) return 0;

  chunk = EmitMakeCall(chunk, returns);
  chunk->src = src;

  return chunk;
}

static Chunk *CompileMember(ASTNode *node, bool returns, Compiler *c)
{
  Chunk *chunk;
  ASTNode *record = CloneNode(NodeChild(node, 0));
  ASTNode *key = NodeChild(node, 1);
  ASTNode *call = NewNode(callNode, node->start, node->end, 0);
  ASTNode *fn = NewNode(refNode, node->start, node->end, 0);
  ASTNode *args = NewNode(tupleNode, node->start, node->end, 0);

  key = NewNode(symNode, key->start, key->end, IntVal(NodeValue(key)));
  NodePush(fn, NewNode(idNode, node->start, node->end, Symbol("Record")));
  NodePush(fn, NewNode(idNode, node->start, node->end, Symbol("get")));
  NodePush(call, fn);
  NodePush(args, record);
  NodePush(args, key);
  NodePush(call, args);

  chunk = CompileExpr(call, returns, c);
  FreeNode(call);
  return chunk;
}

static Chunk *CompileDot(ASTNode *node, bool returns, Compiler *c)
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

  i32 pos = EnvFind(NodeValue(alias), c->env);
  if (pos >= 0) return CompileMember(node, returns, c);

  return CompileRef(alias, sym, returns, c);
}

static Chunk *CompileRef(
  ASTNode *alias, ASTNode *sym, bool returns, Compiler *c)
{
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

  chunk = NewChunk(sym->start);
  EmitGetMod(chunk);
  EmitConst(mod->id - 1, chunk);
  Emit(opGet, chunk);
  WriteCheckMod(chunk);
  EmitConst(sym_index, chunk);
  Emit(opGet, chunk);
  if (returns) EmitReturn(chunk);
  return chunk;
}

static Chunk *CompileModule(ASTNode *node, bool returns, Compiler *c)
{
  /*
  <body>

  ; if not main function, set exports
  getMod
  const <mod id>
  rot
  set
  drop
  */

  ASTNode *imports = NodeChild(node, 1);
  u32 i;
  Chunk *chunk;

  InitHashMap(&c->alias_map);
  for (i = 0; i < NodeCount(imports); i++) {
    ASTNode *import = NodeChild(imports, i);
    u32 name = NodeValue(NodeChild(import, 0));
    u32 alias = NodeValue(NodeChild(import, 1));
    u32 mod_index;
    if (name == Symbol("Host")) {
      u32 j;
      ASTNode *fns = NodeChild(import, 2);
      for (j = 0; j < NodeCount(fns); j++) {
        u32 fn = NodeValue(NodeChild(fns, j));
        HashMapSet(&c->host_imports, fn, PrimitiveID(fn));
      }
      continue;
    }
    if (!HashMapContains(c->mod_map, name)) {
      return UndefinedModule(NodeChild(import, 0), c);
    }
    mod_index = HashMapGet(c->mod_map, name);
    HashMapSet(&c->alias_map, alias, mod_index);
  }

  chunk = CompileExpr(NodeChild(node, 3), false, c);
  if (!chunk) return 0;

  if (c->mod_id > 0) {
    EmitSetModule(chunk, c->mod_id);
  }

  DestroyHashMap(&c->alias_map);

  return chunk;
}

static Chunk *CompileExpr(ASTNode *node, bool returns, Compiler *c)
{
  switch (node->type) {
  case idNode:      return CompileVar(node, returns, c);
  case constNode:   return CompileConst(node, returns, c);
  case symNode:     return CompileConst(node, returns, c);
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
  case xorNode:     return CompileOp(opXor, node, returns, c);
  case gtNode:      return CompileOp(opGt, node, returns, c);
  case joinNode:    return CompileOp(opJoin, node, returns, c);
  case sliceNode:   return CompileOp(opSlice, node, returns, c);
  case bitorNode:   return CompileOp(opOr, node, returns, c);
  case pairNode:    return CompileOp(opPair, node, returns, c);
  case andNode:     return CompileLogic(node, returns, c);
  case orNode:      return CompileLogic(node, returns, c);
  case accessNode:  return CompileOp(opGet, node, returns, c);
  case callNode:    return CompileCall(node, returns, c);
  case refNode:     return CompileDot(node, returns, c);
  case ifNode:      return CompileIf(node, returns, c);
  case letNode:     return CompileLet(node, returns, c);
  case assignNode:  return CompileAssign(node, returns, c);
  case doNode:      return CompileDo(node, returns, c);
  case moduleNode:  return CompileModule(node, returns, c);
  default:          assert(false);
  }
}

Chunk *Compile(ASTNode *node, Compiler *c)
{
  return CompileExpr(node, false, c);
}
