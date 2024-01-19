#include "compile.h"
#include "project.h"
#include "runtime/ops.h"
#include "runtime/primitives.h"

#define LinkNext false
#define LinkReturn true

static Result CompileExpr(Node *node, bool linkage, Compiler *c);
static void CompileLinkage(bool linkage, Compiler *c);
static Result CompileError(char *message, Compiler *c);
static void PushNodePos(Node *node, Compiler *c);
static u32 LambdaBegin(u32 num_params, Compiler *c);
static void LambdaEnd(u32 ref, Compiler *c);
#define CompileOk()  ValueResult(Nil)

void InitCompiler(Compiler *c, SymbolTable *symbols, ObjVec *modules, HashMap *mod_map, Chunk *chunk)
{
  c->pos = 0;
  c->env = 0;
  c->filename = 0;
  c->symbols = symbols;
  c->modules = modules;
  c->mod_map = mod_map;
  c->chunk = chunk;
  c->mod_num = 0;
}

/* If there are multiple modules in a chunk, the first thing a program does is
extend the base environment (which defines the primitives) with a new frame,
where the modules will be defined. */
Result CompileModuleFrame(u32 num_modules, Compiler *c)
{
  if (num_modules > 0) {
    PushFilePos(Sym("*init*", &c->chunk->symbols), c->chunk);
    PushConst(IntVal(num_modules), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
  }

  return CompileOk();
}

static Result CompileImports(Node *imports, Compiler *c);

Result CompileModule(Node *module, Compiler *c)
{
  Result result;
  Node *imports = ModuleImports(module);
  Node *body = ModuleBody(module);
  u32 num_imports = NumNodeChildren(imports);
  u32 lambda_ref;

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  PushFilePos(ModuleFile(module), c->chunk);

  /* create lambda for module */
  if (ModuleName(module) != MainModule) {
    lambda_ref = LambdaBegin(0, c);
  }

  /* compile imports */
  if (num_imports > 0) {
    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  result = CompileExpr(body, LinkNext, c);
  if (!result.ok) return result;
  PushNodePos(ModuleFilename(module), c);

  /* discard import frame */
  if (num_imports > 0) {
    PushByte(OpExport, c->chunk);
    PushByte(OpPop, c->chunk);
    c->env = PopFrame(c->env);
  }

  if (ModuleName(module) != MainModule) {
    PushByte(OpReturn, c->chunk);
    LambdaEnd(lambda_ref, c);

    /* define module as the lambda */
    PushConst(IntVal(c->mod_num), c->chunk);
    PushByte(OpDefine, c->chunk);
    c->mod_num++;
  } else {
    PushByte(OpHalt, c->chunk);
  }

  return CompileOk();
}

static u32 DefineImports(Node *imports, u32 num_imported, Compiler *c)
{
  u32 i;

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Val import = NodeValue(NodeChild(imports, i));

    if (i < NumNodeChildren(imports) - 1) {
      /* keep map for future iterations */
      PushByte(OpDup, c->chunk);
    }
    PushConst(import, c->chunk);
    PushByte(OpSwap, c->chunk);
    PushConst(IntVal(1), c->chunk);
    PushByte(OpApply, c->chunk);
    PushConst(IntVal(num_imported+i), c->chunk);
    PushByte(OpDefine, c->chunk);
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
  PushConst(IntVal(num_imports), c->chunk);
  PushByte(OpTuple, c->chunk);
  PushByte(OpExtend, c->chunk);
  c->env = ExtendFrame(c->env, num_imports);

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Node *node = NodeChild(imports, i);
    Val import_name = NodeValue(NodeChild(node, 0));
    Val alias = NodeValue(NodeChild(node, 1));
    Node *import_mod;
    Node *imported_vals;
    i32 import_def;
    u32 link, link_ref;

    PushNodePos(node, c);

    if (!HashMapContains(c->mod_map, import_name)) return CompileError("Undefined module", c);

    import_mod = VecRef(c->modules, HashMapGet(c->mod_map, import_name));
    imported_vals = ModuleExports(import_mod);

    import_def = FrameFind(c->env, import_name);
    if (import_def < 0) return CompileError("Undefined module", c);

    /* call module function */
    link = PushConst(IntVal(0), c->chunk);
    link_ref = PushByte(OpLink, c->chunk);
    PushConst(IntVal(import_def), c->chunk);
    PushByte(OpLookup, c->chunk);
    PushConst(IntVal(0), c->chunk);
    PushByte(OpApply, c->chunk);
    PatchConst(c->chunk, link, link_ref);

    if (alias == Nil) {
      /* import directly into module namespace */
      num_imported += DefineImports(imported_vals, num_imported, c);
    } else {
      /* import map */
      PushConst(IntVal(num_imported), c->chunk);
      PushByte(OpDefine, c->chunk);
      FrameSet(c->env, num_imported, alias);
      num_imported++;
    }
  }

  return CompileOk();
}

static Result CompileDo(Node *expr, bool linkage, Compiler *c);
static Result CompileExport(Node *node, bool linkage, Compiler *c);
static Result CompileCall(Node *call, bool linkage, Compiler *c);
static Result CompileSet(Node *node, bool linkage, Compiler *c);
static Result CompileLambda(Node *expr, bool linkage, Compiler *c);
static Result CompileIf(Node *expr, bool linkage, Compiler *c);
static Result CompileAnd(Node *expr, bool linkage, Compiler *c);
static Result CompileOr(Node *expr, bool linkage, Compiler *c);
static Result CompileList(Node *items, bool linkage, Compiler *c);
static Result CompileTuple(Node *items, bool linkage, Compiler *c);
static Result CompileMap(Node *items, bool linkage, Compiler *c);
static Result CompileString(Node *sym, bool linkage, Compiler *c);
static Result CompileVar(Node *id, bool linkage, Compiler *c);
static Result CompileConst(Node *value, bool linkage, Compiler *c);

static Result CompileExpr(Node *node, bool linkage, Compiler *c)
{
  PushNodePos(node, c);

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
  case AndNode:           return CompileAnd(node, linkage, c);
  case OrNode:            return CompileOr(node, linkage, c);
  case ExportNode:        return CompileExport(node, linkage, c);
  default:                return CompileError("Unknown expression", c);
  }
}

static Result CompileDo(Node *node, bool linkage, Compiler *c)
{

  Node *assigns = NodeChild(node, 0);
  Node *stmts = NodeChild(node, 1);
  u32 num_assigns = NumNodeChildren(assigns);
  u32 num_assigned = 0;
  u32 i;

  if (num_assigns > 0) {
    PushConst(IntVal(num_assigns), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
    c->env = ExtendFrame(c->env, num_assigns);
  }

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
      Node *var = NodeChild(stmt, 0);
      Node *value = NodeChild(stmt, 1);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushNodePos(var, c);
      PushConst(IntVal(num_assigned), c->chunk);
      PushByte(OpDefine, c->chunk);
      FrameSet(c->env, num_assigned, NodeValue(var));
      num_assigned++;

      if (is_last) {
        /* the last statement must produce a result */
        PushConst(Nil, c->chunk);
      }
    } else if (stmt->type == DefNode) {
      Node *var = NodeChild(stmt, 0);
      Node *value = NodeChild(stmt, 1);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushNodePos(var, c);
      PushConst(IntVal(num_assigned), c->chunk);
      PushByte(OpDefine, c->chunk);
      num_assigned++;

      if (is_last) {
        /* the last statement must produce a result */
        PushConst(Nil, c->chunk);
      }
    } else {
      result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;
      if (!is_last) {
        /* discard each statement result except the last */
        PushByte(OpPop, c->chunk);
      }
    }
  }

  /* discard frame for assigns */
  PushNodePos(node, c);
  if (num_assigns > 0) {
    PushByte(OpExport, c->chunk);
    PushByte(OpPop, c->chunk);
    c->env = PopFrame(c->env);
  }

  return CompileOk();
}

static Result CompileExport(Node *node, bool linkage, Compiler *c)
{
  u32 num_exports = NumNodeChildren(node);
  u32 exports_frame;

  PushByte(OpMap, c->chunk);
  if (num_exports > 0) {
    u32 i;

    for (i = 0; i < num_exports; i++) {
      Val name = NodeValue(NodeChild(node, i));
      Sym(SymbolName(name, c->symbols), &c->chunk->symbols);
      PushConst(IntVal(i), c->chunk);
      PushByte(OpLookup, c->chunk);
      PushConst(name, c->chunk);
      PushByte(OpPut, c->chunk);
    }
  }

  PushByte(OpDup, c->chunk);

  /* redefine module as exported value */
  exports_frame = ExportsFrame(c->env);
  PushConst(IntVal(exports_frame + c->mod_num), c->chunk);
  PushByte(OpDefine, c->chunk);
  CompileLinkage(linkage, c);

  return CompileOk();
}

static bool IsPrimitiveCall(Node *op, Compiler *c)
{
  /* primitive calls are calls from the highest env frame */
  if (op->type != IDNode) return false;
  return FrameNum(c->env, NodeValue(op)) == 0;
}

static Result CompileCall(Node *node, bool linkage, Compiler *c)
{
  Node *op = NodeChild(node, 0);
  u32 num_args = NumNodeChildren(node) - 1;
  u32 i, link, link_ref;
  Result result;
  bool is_primitive;

  if (op->type == IDNode && NodeValue(op) == SymSet) {
    return CompileSet(node, linkage, c);
  }

  is_primitive = IsPrimitiveCall(op, c);

  if (linkage == LinkNext && !is_primitive) {
    link = PushConst(IntVal(0), c->chunk);
    link_ref = PushByte(OpLink, c->chunk);
  }

  for (i = 0; i < num_args; i++) {
    Node *arg = NodeChild(node, i + 1);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
  }

  result = CompileExpr(op, LinkNext, c);
  if (!result.ok) return result;

  PushConst(IntVal(num_args), c->chunk);
  PushByte(OpApply, c->chunk);

  if (linkage == LinkNext && !is_primitive) {
    PatchConst(c->chunk, link, link_ref);
  }

  if (is_primitive) {
    CompileLinkage(linkage, c);
  }

  return CompileOk();
}

static Result CompileSet(Node *node, bool linkage, Compiler *c)
{
  u32 num_args = NumNodeChildren(node) - 1;
  Node *var, *val;
  Val id;
  i32 def;
  Result result;

  if (num_args != 2) {
    return CompileError("Wrong number of arguments: Expected 2", c);
  }

  var = NodeChild(node, 1);
  val = NodeChild(node, 2);

  if (var->type != IDNode) {
    return CompileError("Expected variable", c);
  }

  id = NodeValue(var);
  def = FrameFind(c->env, id);
  if (def == -1) return CompileError("Undefined variable", c);

  result = CompileExpr(val, LinkNext, c);
  if (!result.ok) return result;

  PushByte(OpDup, c->chunk);
  PushConst(IntVal(def), c->chunk);
  PushByte(OpDefine, c->chunk);

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileLambda(Node *node, bool linkage, Compiler *c)
{
  Node *params = NodeChild(node, 0);
  Node *body = NodeChild(node, 1);
  Result result;
  u32 lambda_ref, i;
  u32 num_params = NumNodeChildren(params);

  /* create lambda */
  lambda_ref = LambdaBegin(num_params, c);

  if (num_params > 0) {
    PushNodePos(params, c);
    PushConst(IntVal(num_params), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
    c->env = ExtendFrame(c->env, num_params);

    for (i = 0; i < num_params; i++) {
      Node *param = NodeChild(params, i);
      Val id = NodeValue(NodeChild(params, i));
      PushNodePos(param, c);
      FrameSet(c->env, i, id);
      PushConst(IntVal(num_params - i - 1), c->chunk);
      PushByte(OpDefine, c->chunk);
    }
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  LambdaEnd(lambda_ref, c);

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
  u32 branch, branch_ref, jump, jump_ref;

  result = CompileExpr(predicate, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(alternative, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->chunk);
    jump_ref = PushByte(OpJump, c->chunk);
  }

  PatchConst(c->chunk, branch, branch_ref);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(consequent, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump, jump_ref);
  }

  return CompileOk();
}

static Result CompileAnd(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch, branch_ref, jump, jump_ref;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->chunk);
    jump_ref = PushByte(OpJump, c->chunk);
  } else {
    CompileLinkage(linkage, c);
  }

  PatchConst(c->chunk, branch, branch_ref);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump, jump_ref);
  }

  return CompileOk();
}

static Result CompileOr(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch, branch_ref;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  PatchConst(c->chunk, branch, branch_ref);
  CompileLinkage(linkage, c);

  return CompileOk();
}

static Result CompileList(Node *node, bool linkage, Compiler *c)
{
  u32 i;
  PushConst(Nil, c->chunk);

  for (i = 0; i < NumNodeChildren(node); i++) {
    Node *item = NodeChild(node, NumNodeChildren(node) - i - 1);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileTuple(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node);

  PushConst(IntVal(num_items), c->chunk);
  PushByte(OpTuple, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *item = NodeChild(node, i);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->chunk);
    PushByte(OpSet, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileMap(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node) / 2;

  PushByte(OpMap, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *value = NodeChild(node, (i*2)+1);
    Node *key = NodeChild(node, i*2);

    Result result = CompileExpr(value, LinkNext, c);
    if (!result.ok) return result;

    result = CompileExpr(key, LinkNext, c);
    if (!result.ok) return result;

    PushByte(OpPut, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileString(Node *node, bool linkage, Compiler *c)
{
  Val sym = NodeValue(node);
  Sym(SymbolName(sym, c->symbols), &c->chunk->symbols);
  PushConst(sym, c->chunk);
  PushByte(OpStr, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileVar(Node *node, bool linkage, Compiler *c)
{
  Val id = NodeValue(node);
  i32 def = FrameFind(c->env, id);
  if (def == -1) return CompileError("Undefined variable", c);

  PushConst(IntVal(def), c->chunk);
  PushByte(OpLookup, c->chunk);

  if (IsPrimitiveCall(node, c)) {
    /* TODO: wrap the primitive in a lambda, so it returns correctly when called */
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileConst(Node *node, bool linkage, Compiler *c)
{
  Val value = NodeValue(node);
  PushConst(value, c->chunk);
  CompileLinkage(linkage, c);
  if (IsSym(value)) {
    Sym(SymbolName(value, c->symbols), &c->chunk->symbols);
  }
  return CompileOk();
}

static void CompileLinkage(bool linkage, Compiler *c)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, c->chunk);
  }
}

static Result CompileError(char *message, Compiler *c)
{
  return ErrorResult(message, c->filename, c->pos);
}

static void PushNodePos(Node *node, Compiler *c)
{
  PushSourcePos(node->pos - c->pos, c->chunk);
  c->pos = node->pos;
}

static u32 LambdaBegin(u32 num_params, Compiler *c)
{
  u32 lambda, lambda_ref, ref;
  lambda = PushConst(IntVal(0), c->chunk);
  PushConst(IntVal(num_params), c->chunk);
  lambda_ref = PushByte(OpLambda, c->chunk);
  PushConst(IntVal(0), c->chunk);
  ref = PushByte(OpJump, c->chunk);
  PatchConst(c->chunk, lambda, lambda_ref);
  return ref;
}

static void LambdaEnd(u32 ref, Compiler *c)
{
  PatchConst(c->chunk, ref - 2, ref);
}
