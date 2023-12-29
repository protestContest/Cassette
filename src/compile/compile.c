#include "compile.h"
#include "parse.h"
#include "project.h"
#include "runtime/ops.h"
#include "runtime/primitives.h"

#define LinkNext false
#define LinkReturn true

static Result CompileImports(Node *imports, Compiler *c);
static Result CompileExpr(Node *node, bool linkage, Compiler *c);
static Result CompileStmts(Node *node, bool linkage, Compiler *c);
static Result CompileDo(Node *expr, bool linkage, Compiler *c);
static Result CompileCall(Node *call, bool linkage, Compiler *c);
static Result CompileLambda(Node *expr, bool linkage, Compiler *c);
static Result CompileIf(Node *expr, bool linkage, Compiler *c);
static Result CompileAnd(Node *expr, bool linkage, Compiler *c);
static Result CompileOr(Node *expr, bool linkage, Compiler *c);
static Result CompileOp(OpCode op, Node *expr, bool linkage, Compiler *c);
static Result CompileNotOp(OpCode op, Node *expr, bool linkage, Compiler *c);
static Result CompileNegOp(OpCode op, Node *args, bool linkage, Compiler *c);
static Result CompileList(Node *items, bool linkage, Compiler *c);
static Result CompileTuple(Node *items, bool linkage, Compiler *c);
static Result CompileMap(Node *items, bool linkage, Compiler *c);
static Result CompileString(Node *sym, bool linkage, Compiler *c);
static Result CompileVar(Node *id, bool linkage, Compiler *c);
static Result CompileConst(Node *value, bool linkage, Compiler *c);

static void CompileLinkage(bool linkage, Compiler *c);
static Result CompileError(char *message, Compiler *c);
#define CompileOk()  OkResult(Nil)

void InitCompiler(Compiler *c, SymbolTable *symbols, ObjVec *modules, HashMap *mod_map, Chunk *chunk)
{
  c->pos = 0;
  c->env = 0;
  c->filename = 0;
  c->symbols = symbols;
  c->modules = modules;
  c->mod_map = mod_map;
  c->chunk = chunk;
}

Result CompileInitialEnv(u32 num_modules, Compiler *c)
{
  PrimitiveModuleDef *primitives = GetPrimitives();
  u32 num_primitives = NumPrimitives();
  u32 frame_size = num_primitives + num_modules;
  Frame *env;
  u32 mod;

  c->filename = "*startup*";
  c->pos = 0;
  BeginChunkFile(Sym(c->filename, &c->chunk->symbols), c->chunk);

  PushConst(IntVal(frame_size), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  PushByte(OpExtend, c->pos, c->chunk);
  env = ExtendFrame(0, frame_size);

  for (mod = 0; mod < num_primitives; mod++) {
    u32 fn;

    /* create tuples for module */
    PushConst(IntVal(primitives[mod].num_fns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);

    for (fn = 0; fn < primitives[mod].num_fns; fn++) {
      /*  build primitive reference */
      PushConst(Nil, c->pos, c->chunk);
      PushConst(IntVal(fn), c->pos, c->chunk);
      PushByte(OpPair, c->pos, c->chunk);
      PushConst(IntVal(mod), c->pos, c->chunk);
      PushByte(OpPair, c->pos, c->chunk);
      PushConst(Primitive, c->pos, c->chunk);
      PushByte(OpPair, c->pos, c->chunk);

      /* add to tuple */
      PushConst(IntVal(fn), c->pos, c->chunk);
      PushByte(OpSet, c->pos, c->chunk);
    }

    PushConst(IntVal(primitives[mod].num_fns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);

    for (fn = 0; fn < primitives[mod].num_fns; fn++) {
      PushConst(primitives[mod].fns[fn].name, c->pos, c->chunk);
      PushConst(IntVal(fn), c->pos, c->chunk);
      PushByte(OpSet, c->pos, c->chunk);
    }

    PushByte(OpPair, c->pos, c->chunk);

    /* define primitive module */
    PushConst(IntVal(num_modules + mod), c->pos, c->chunk);
    PushByte(OpDefine, c->pos, c->chunk);

    FrameSet(env, num_modules + mod, primitives[mod].module);
  }

  EndChunkFile(c->chunk);

  return DataResult(env);
}

Result CompileScript(Node *module, Compiler *c)
{
  Result result;
  Node *imports = ModuleImports(module);
  u32 num_imports = NumNodeChildren(imports);

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  BeginChunkFile(ModuleFile(module), c->chunk);

  /* compile imports */
  if (num_imports > 0) {
    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  result = CompileDo(ModuleBody(module), LinkNext, c);
  if (!result.ok) return result;

  /* discard import frame */
  if (num_imports > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = PopFrame(c->env);
  }

  EndChunkFile(c->chunk);

  return CompileOk();
}

Result CompileModule(Node *module, u32 mod_num, Compiler *c)
{
  Result result;
  Node *imports = ModuleImports(module);
  Node *exports = ModuleExports(module);
  u32 num_imports = NumNodeChildren(imports);
  u32 num_exports = NumNodeChildren(exports);
  u32 jump, link;

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  BeginChunkFile(ModuleFile(module), c->chunk);

  /* create lambda for module */
  PushConst(IntVal(0), c->pos, c->chunk);
  link = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpLambda, c->pos, c->chunk);
  jump = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpJump, c->pos, c->chunk);
  PatchConst(c->chunk, link);

  /* compile imports */
  if (num_imports > 0) {
    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  /* extend env for assigns */
  if (num_exports > 0) {
    PushConst(IntVal(num_exports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendFrame(c->env, num_exports);
  }

  result = CompileStmts(ModuleBody(module), LinkNext, c);
  if (!result.ok) return result;

  /* discard last statement result */
  PushByte(OpPop, c->pos, c->chunk);

  /* export assigned values */
  if (num_exports > 0) {
    u32 i;
    PushByte(OpExport, c->pos, c->chunk);

    /* build tuple of keys */
    PushConst(IntVal(num_exports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);

    for (i = 0; i < num_exports; i++) {
      Val export = NodeValue(NodeChild(exports, i));
      Sym(SymbolName(export, c->symbols), &c->chunk->symbols);
      PushConst(export, c->pos, c->chunk);
      PushConst(IntVal(i), c->pos, c->chunk);
      PushByte(OpSet, c->pos, c->chunk);
    }
    PushByte(OpPair, c->pos, c->chunk);
    c->env = PopFrame(c->env);
  } else {
    /* empty map */
    PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpPair, c->pos, c->chunk);
  }
  /* copy exports to redefine module */
  PushByte(OpDup, c->pos, c->chunk);

  /* discard import frame */
  if (num_imports > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = PopFrame(c->env);
  }

  /* redefine module as exported value */
  PushConst(IntVal(mod_num), c->pos, c->chunk);
  PushByte(OpDefine, c->pos, c->chunk);

  PushByte(OpReturn, c->pos, c->chunk);

  PatchConst(c->chunk, jump);

  /* define module as the lambda */
  PushConst(IntVal(mod_num), c->pos, c->chunk);
  PushByte(OpDefine, c->pos, c->chunk);

  EndChunkFile(c->chunk);

  return CompileOk();
}

static u32 DefineImports(Node *imports, u32 num_imported, Compiler *c)
{
  u32 i;

  PushByte(OpUnpair, c->pos, c->chunk);
  PushByte(OpPop, c->pos, c->chunk);

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Val import = NodeValue(NodeChild(imports, i));

    if (i < NumNodeChildren(imports) - 1) {
      /* keep tuple for future iterations */
      PushByte(OpDup, c->pos, c->chunk);
    }
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpGet, c->pos, c->chunk);
    PushConst(IntVal(num_imported+i), c->pos, c->chunk);
    PushByte(OpDefine, c->pos, c->chunk);
    FrameSet(c->env, num_imported + i, import);
  }
  return i;
}

static u32 CountExports(Node *imports, Compiler *c)
{
  u32 count = 0;
  u32 i;
  for (i = 0; i < NumNodeChildren(imports); i++) {
    Node *import = NodeChild(imports, i);
    Val mod_name = NodeValue(NodeChild(import, 0));
    Val alias = NodeValue(NodeChild(import, 1));

    if (alias != Nil) {
      count++;
    } else if (HashMapContains(c->mod_map, mod_name)) {
      Node *module = VecRef(c->modules, HashMapGet(c->mod_map, mod_name));
      Node *exports = ModuleExports(module);
      count += NumNodeChildren(exports);
    }
  }
  return count;
}

static Result CompileImports(Node *imports, Compiler *c)
{
  u32 num_imported = 0;
  u32 num_imports = CountExports(imports, c);
  u32 i;

  /* extend env for imports */
  PushConst(IntVal(num_imports), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  PushByte(OpExtend, c->pos, c->chunk);
  c->env = ExtendFrame(c->env, num_imports);

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Node *node = NodeChild(imports, i);
    Val import_name = NodeValue(NodeChild(node, 0));
    Val alias = NodeValue(NodeChild(node, 1));
    Node *import_mod;
    Node *imported_vals;
    i32 import_def;
    u32 link;

    c->pos = node->pos;

    if (!HashMapContains(c->mod_map, import_name)) return CompileError("Undefined module", c);

    import_mod = VecRef(c->modules, HashMapGet(c->mod_map, import_name));
    imported_vals = ModuleExports(import_mod);

    import_def = FrameFind(c->env, import_name);
    if (import_def < 0) return CompileError("Undefined module", c);

    /* call module function */
    link = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpLink, c->pos, c->chunk);
    PushConst(IntVal(import_def), c->pos, c->chunk);
    PushByte(OpLookup, c->pos, c->chunk);
    PushByte(OpApply, c->pos, c->chunk);
    PushByte(0, c->pos, c->chunk);
    PatchConst(c->chunk, link);

    if (alias == Nil) {
      /* import directly into module namespace */
      num_imported += DefineImports(imported_vals, num_imported, c);
    } else {
      /* import into a map */
      PushByte(OpUnpair, c->pos, c->chunk);
      PushByte(OpMap, c->pos, c->chunk);

      PushConst(IntVal(num_imported), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      FrameSet(c->env, num_imported, alias);
      num_imported++;
    }
  }

  return CompileOk();
}

static Result CompileExpr(Node *node, bool linkage, Compiler *c)
{
  c->pos = node->pos;

  switch (node->type) {
  case SymbolNode:        return CompileConst(node, linkage, c);
  case DoNode:            return CompileDo(node, linkage, c);
  case LambdaNode:        return CompileLambda(node, linkage, c);
  case CallNode:          return CompileCall(node, linkage, c);
  case NilNode:           return CompileConst(node, linkage, c);
  case IfNode:            return CompileIf(node, linkage, c);
  case ListNode:          return CompileList(node, linkage, c);
  case MapNode:           return CompileMap(node, linkage, c);
  case TupleNode:         return CompileTuple(node, linkage, c);
  case IDNode:            return CompileVar(node, linkage, c);
  case NumNode:           return CompileConst(node, linkage, c);
  case StringNode:        return CompileString(node, linkage, c);
  case NotEqNode:         return CompileNotOp(OpEq, node, linkage, c);
  case RemNode:           return CompileOp(OpRem, node, linkage, c);
  case BitAndNode:        return CompileOp(OpBitAnd, node, linkage, c);
  case MultiplyNode:      return CompileOp(OpMul, node, linkage, c);
  case AddNode:           return CompileOp(OpAdd, node, linkage, c);
  case SubtractNode:      return CompileOp(OpSub, node, linkage, c);
  case DivideNode:        return CompileOp(OpDiv, node, linkage, c);
  case LtNode:            return CompileOp(OpLt, node, linkage, c);
  case LShiftNode:        return CompileOp(OpShift, node, linkage, c);
  case LtEqNode:          return CompileNotOp(OpGt, node, linkage, c);
  case CatNode:           return CompileOp(OpCat, node, linkage, c);
  case EqNode:            return CompileOp(OpEq, node, linkage, c);
  case GtNode:            return CompileOp(OpGt, node, linkage, c);
  case GtEqNode:          return CompileNotOp(OpLt, node, linkage, c);
  case RShiftNode:        return CompileNegOp(OpShift, node, linkage, c);
  case BitOrNode:         return CompileOp(OpBitOr, node, linkage, c);
  case AndNode:           return CompileAnd(node, linkage, c);
  case InNode:            return CompileOp(OpIn, node, linkage, c);
  case OrNode:            return CompileOr(node, linkage, c);
  case PairNode:          return CompileOp(OpPair, node, linkage, c);
  case LengthNode:        return CompileOp(OpLen, node, linkage, c);
  case NegativeNode:      return CompileOp(OpNeg, node, linkage, c);
  case NotNode:           return CompileOp(OpNot, node, linkage, c);
  case BitNotNode:        return CompileOp(OpBitNot, node, linkage, c);
  default:                return CompileError("Unknown expression", c);
  }
}

static Result CompileStmts(Node *node, bool linkage, Compiler *c)
{
  Node *stmts = NodeChild(node, 1);
  u32 num_assigned = 0;
  u32 i;

  /* pre-define all def statements */
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    Val var;

    /* keep the assignment slots aligned */
    if (stmt->type == LetNode) {
      num_assigned++;
      continue;
    }
    if (stmt->type != DefNode) continue;

    var = NodeValue(NodeChild(stmt, 0));
    FrameSet(c->env, num_assigned, var);
    num_assigned++;
  }

  num_assigned = 0;
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    Result result;
    bool is_last = i == NumNodeChildren(stmts) - 1;
    bool stmt_linkage = is_last ? linkage : LinkNext;

    if (stmt->type == LetNode) {
      Val var = NodeValue(NodeChild(stmt, 0));
      Node *value = NodeChild(stmt, 1);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushConst(IntVal(num_assigned), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      FrameSet(c->env, num_assigned, var);
      num_assigned++;

      if (is_last) {
        /* the last statement must produce a result */
        PushConst(Nil, c->pos, c->chunk);
      }
    } else if (stmt->type == DefNode) {
      Node *value = NodeChild(stmt, 1);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushConst(IntVal(num_assigned), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      num_assigned++;

      if (is_last) {
        /* the last statement must produce a result */
        PushConst(Nil, c->pos, c->chunk);
      }
    } else {
      result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;
      if (!is_last) {
        /* discard each statement result except the last */
        PushByte(OpPop, c->pos, c->chunk);
      }
    }
  }

  return CompileOk();
}

static Result CompileDo(Node *node, bool linkage, Compiler *c)
{
  Result result;
  Node *assigns = NodeChild(node, 0);
  u32 num_assigns = NumNodeChildren(assigns);

  if (num_assigns > 0) {
    PushConst(IntVal(num_assigns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendFrame(c->env, num_assigns);
  }

  result = CompileStmts(node, linkage, c);
  if (!result.ok) return result;

  /* discard frame for assigns */
  if (num_assigns > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = PopFrame(c->env);
  }

  return CompileOk();
}

static Result CompileCall(Node *node, bool linkage, Compiler *c)
{
  Node *op = NodeChild(node, 0);
  u32 num_args = NumNodeChildren(node) - 1;
  u32 i, patch;
  Result result;

  if (linkage != LinkReturn) {
    patch = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpLink, c->pos, c->chunk);
  }

  for (i = 0; i < num_args; i++) {
    Node *arg = NodeChild(node, i + 1);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
  }

  result = CompileExpr(op, LinkNext, c);
  if (!result.ok) return result;

  PushByte(OpApply, c->pos, c->chunk);
  PushByte(num_args, c->pos, c->chunk);

  if (linkage == LinkNext) {
    PatchConst(c->chunk, patch);
  }

  return CompileOk();
}

static Result CompileLambda(Node *node, bool linkage, Compiler *c)
{
  Node *params = NodeChild(node, 0);
  Node *body = NodeChild(node, 1);
  Result result;
  u32 jump, link, i;
  u32 num_params = NumNodeChildren(params);

  /* create lambda */
  PushConst(IntVal(num_params), c->pos, c->chunk);
  link = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpLambda, c->pos, c->chunk);
  jump = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpJump, c->pos, c->chunk);
  PatchConst(c->chunk, link);

  if (num_params > 0) {
    PushConst(IntVal(num_params), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendFrame(c->env, num_params);

    for (i = 0; i < num_params; i++) {
      Val param = NodeValue(NodeChild(params, i));
      FrameSet(c->env, i, param);
      PushConst(IntVal(num_params - i - 1), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
    }
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  PatchConst(c->chunk, jump);

  CompileLinkage(linkage, c);

  if (num_params > 0) c->env = PopFrame(c->env);

  return CompileOk();
}

static Result CompileIf(Node *node, bool linkage, Compiler *c)
{
  Node *predicate = NodeChild(node, 0);
  Node *consequent = NodeChild(node, 1);
  Node *alternative = NodeChild(node, 2);
  Result result;
  u32 branch, jump;

  result = CompileExpr(predicate, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpBranch, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(alternative, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpJump, c->pos, c->chunk);
  }

  PatchConst(c->chunk, branch);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(consequent, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump);
  }

  return CompileOk();
}

static Result CompileAnd(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch, jump;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpBranch, c->pos, c->chunk);

  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpJump, c->pos, c->chunk);
  } else {
    CompileLinkage(linkage, c);
  }

  PatchConst(c->chunk, branch);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump);
  }

  return CompileOk();
}

static Result CompileOr(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpBranch, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  PatchConst(c->chunk, branch);
  CompileLinkage(linkage, c);

  return CompileOk();
}

static Result CompileOp(OpCode op, Node *node, bool linkage, Compiler *c)
{
  u32 pos = c->pos;
  u32 i;
  for (i = 0; i < NumNodeChildren(node); i++) {
    Result result = CompileExpr(NodeChild(node, i), LinkNext, c);
    if (!result.ok) return result;
  }
  PushByte(op, pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileNotOp(OpCode op, Node *node, bool linkage, Compiler *c)
{
  u32 i;
  for (i = 0; i < NumNodeChildren(node); i++) {
    Result result = CompileExpr(NodeChild(node, i), LinkNext, c);
    if (!result.ok) return result;
  }
  PushByte(op, c->pos, c->chunk);
  PushByte(OpNot, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileNegOp(OpCode op, Node *node, bool linkage, Compiler *c)
{
  u32 i;
  for (i = 0; i < NumNodeChildren(node); i++) {
    Result result = CompileExpr(NodeChild(node, i), LinkNext, c);
    if (!result.ok) return result;
  }
  PushByte(OpNeg, c->pos, c->chunk);
  PushByte(op, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileList(Node *node, bool linkage, Compiler *c)
{
  u32 i;
  PushConst(Nil, c->pos, c->chunk);

  for (i = 0; i < NumNodeChildren(node); i++) {
    Node *item = NodeChild(node, NumNodeChildren(node) - i - 1);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->pos, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileTuple(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node);

  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *item = NodeChild(node, i);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileMap(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node) / 2;

  /* create tuple for values */
  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *value = NodeChild(node, (i*2)+1);

    Result result = CompileExpr(value, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);
  }

  /* create tuple for keys */
  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *key = NodeChild(node, i*2);

    Result result = CompileExpr(key, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);
  }

  /* create map from tuples */
  PushByte(OpMap, c->pos, c->chunk);

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileString(Node *node, bool linkage, Compiler *c)
{
  Val sym = NodeValue(node);
  Sym(SymbolName(sym, c->symbols), &c->chunk->symbols);
  PushConst(sym, c->pos, c->chunk);
  PushByte(OpStr, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileVar(Node *node, bool linkage, Compiler *c)
{
  Val id = NodeValue(node);
  i32 def = FrameFind(c->env, id);
  if (def == -1) return CompileError("Undefined variable", c);

  PushConst(IntVal(def), c->pos, c->chunk);
  PushByte(OpLookup, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileConst(Node *node, bool linkage, Compiler *c)
{
  Val value = NodeValue(node);
  PushConst(value, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  if (IsSym(value)) {
    Sym(SymbolName(value, c->symbols), &c->chunk->symbols);
  }
  return CompileOk();
}

static void CompileLinkage(bool linkage, Compiler *c)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, c->pos, c->chunk);
  } else if (linkage != LinkNext) {
    PushConst(linkage, c->pos, c->chunk);
    PushByte(OpJump, c->pos, c->chunk);
  }
}

static Result CompileError(char *message, Compiler *c)
{
  return ErrorResult(message, c->filename, c->pos);
}
