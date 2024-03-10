#include "compile.h"
#include "env.h"
#include "module.h"
#include "wasm.h"
#include <univ.h>

typedef struct {
  Module *mod;
  Frame *env;
  u8 **code;
  HashMap *imports;
  Result result;
} Compiler;

#define regFree 0
#define regEnv  1
#define regTmp1 2
#define regTmp2 3
#define regTmp3 4

static i32 allocFn = -1;
static i32 lookupFn = -1;
static i32 makePairFn = -1;
static i32 makeTupleFn = -1;

static i32 CompileFunc(LambdaNode *lambda, Compiler *c);
static bool CompileExpr(Node *expr, Compiler *c);

static bool CompileError(char *msg, u32 pos, Compiler *c)
{
  c->result = Error(msg, c->mod->filename, pos);
  DestroyHashMap(c->imports);
  return false;
}

#define EmitHead(code)  EmitLoad(0, code)
#define EmitTail(code)  EmitLoad(4, code)

static void EmitLambda(i32 funcidx, u8 **code)
{
  EmitConst(funcidx, code);
  EmitGetGlobal(regEnv, code);
  EmitCall(makePairFn, code);
}

static void EmitExtendEnv(i32 size, u8 **code)
{
  EmitGetGlobal(regEnv, code);
  EmitConst(size, code);
  EmitCall(makeTupleFn, code);
  EmitCall(makePairFn, code);
  EmitSetGlobal(regEnv, code);
}

static bool CompileVar(TerminalNode *expr, Compiler *c)
{
  i32 index = FrameFind(c->env, expr->value);
  if (index < 0) return CompileError("Undefined variable", expr->pos, c);

  EmitConst(index, c->code);
  EmitCall(lookupFn, c->code);

  return true;
}

static bool CompileInt(TerminalNode *expr, Compiler *c)
{
  EmitConst(expr->value, c->code);
  return true;
}

static OpCode NodeOp(BinaryNode *node)
{
  switch (node->type) {
  case addNode:     return I32Add;
  case subNode:     return I32Sub;
  case mulNode:     return I32Mul;
  case divNode:     return I32DivS;
  case remNode:     return I32RemS;
  case ltNode:      return I32LtS;
  case ltEqNode:    return I32LeS;
  case gtNode:      return I32GtS;
  case gtEqNode:    return I32GeS;
  case eqNode:      return I32Eq;
  case notEqNode:   return I32Ne;
  default:          return Unreachable;
  }
}

static bool CompileNeg(UnaryNode *expr, Compiler *c)
{
  EmitConst(0, c->code);
  if (!CompileExpr(expr->child, c)) return false;
  EmitByte(I32Sub, c->code);
  return true;
}

static bool CompileNot(UnaryNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->child, c)) return false;
  EmitByte(I32EqZ, c->code);
  return true;
}

static bool CompileBinOp(BinaryNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->right, c)) return false;
  if (!CompileExpr(expr->left, c)) return false;
  EmitByte(NodeOp(expr), c->code);
  return true;
}

static bool CompilePair(BinaryNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->right, c)) return false;
  if (!CompileExpr(expr->left, c)) return false;
  EmitCall(makePairFn, c->code);
  return true;
}

static bool CompileList(ListNode *expr, Compiler *c)
{
  u32 i;
  u32 len = VecCount(expr->items);
  EmitConst(0, c->code);
  for (i = 0; i < len; i++) {
    if (!CompileExpr(expr->items[len-1-i], c)) return false;
    EmitCall(makePairFn, c->code);
  }
  return true;
}

static bool CompileTuple(ListNode *expr, Compiler *c)
{
  u32 i;
  u32 num_items = VecCount(expr->items);

  EmitConst(num_items, c->code);
  EmitCall(makeTupleFn, c->code);

  for (i = 0; i < num_items; i++) {
    EmitSetGlobal(regTmp1, c->code);
    EmitGetGlobal(regTmp1, c->code);
    EmitGetGlobal(regTmp1, c->code);

    if (!CompileExpr(expr->items[i], c)) return false;

    EmitStore(i*4, c->code);
  }

  return true;
}

static bool CompileLambda(LambdaNode *expr, Compiler *c)
{
  i32 func = CompileFunc(expr, c);
  if (func < 0) return false;
  EmitLambda(func, c->code);
  return true;
}

static i32 ImportCallIdx(Node *expr, Compiler *c)
{
  if (expr->type == accessNode) {
    BinaryNode* access = (BinaryNode*)expr;
    if (access->left->type == varNode && access->right->type == symbolNode) {
      TerminalNode *obj = (TerminalNode*)access->left;
      TerminalNode *name = (TerminalNode*)access->right;
      if (HashMapContains(c->imports, obj->value)) {
        return ImportIdx(SymbolName(obj->value), SymbolName(name->value), c->mod);
      }
    }
  }
  return -1;
}

static bool CompileCall(CallNode *expr, Compiler *c)
{
  u32 i;
  i32 import;
  u32 num_args = VecCount(expr->args);

  EmitGetGlobal(regEnv, c->code);
  for (i = 0; i < num_args; i++) {
    if (!CompileExpr(expr->args[num_args-1-i], c)) return false;
  }

  import = ImportCallIdx(expr->op, c);
  if (import >= 0) {
    EmitCall(import, c->code);
  } else {
    EmitSetGlobal(regTmp1, c->code);
    EmitGetGlobal(regTmp1, c->code);
    EmitHead(c->code);
    EmitSetGlobal(regEnv, c->code);
    EmitGetGlobal(regTmp1, c->code);
    EmitTail(c->code);
    EmitCallIndirect(AddType(num_args, 1, c->mod), c->code);
  }

  EmitSetGlobal(regTmp1, c->code);
  EmitSetGlobal(regEnv, c->code);
  EmitGetGlobal(regTmp1, c->code);

  return true;
}

static bool CompileDo(DoNode *expr, Compiler *c)
{
  u32 i;
  u32 num_items = VecCount(expr->stmts);

  for (i = 0; i < num_items; i++) {
    EmitGetGlobal(regEnv, c->code);
    if (!CompileExpr(expr->stmts[i], c)) return false;

    if (i < num_items - 1) {
      EmitByte(DropOp, c->code);
      EmitSetGlobal(regEnv, c->code);
    } else {
      EmitSetGlobal(regTmp1, c->code);
      EmitSetGlobal(regEnv, c->code);
      EmitGetGlobal(regTmp1, c->code);
    }
  }

  return true;
}

static bool CompileExpr(Node *expr, Compiler *c)
{
  printf("Compiling %s (%d)\n", NodeName(expr), expr->pos);

  switch (expr->type) {
  case varNode:       return CompileVar((TerminalNode*)expr, c);
  case intNode:       return CompileInt((TerminalNode*)expr, c);
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:       return CompileBinOp((BinaryNode*)expr, c);
  case notNode:       return CompileNot((UnaryNode*)expr, c);
  case negNode:       return CompileNeg((UnaryNode*)expr, c);
  case pairNode:      return CompilePair((BinaryNode*)expr, c);
  case listNode:      return CompileList((ListNode*)expr, c);
  case tupleNode:     return CompileTuple((ListNode*)expr, c);
  case lambdaNode:    return CompileLambda((LambdaNode*)expr, c);
  case callNode:      return CompileCall((CallNode*)expr, c);
  case doNode:        return CompileDo((DoNode*)expr, c);
  default:            return CompileError("Unknown expr type", expr->pos, c);
  }
}

static i32 CompileFunc(LambdaNode *lambda, Compiler *c)
{
  u32 i;
  u32 num_params = VecCount(lambda->params);
  u32 type = AddType(num_params, 1, c->mod);
  Func *func = AddFunc(type, 0, c->mod);
  u8 **code = c->code;

  c->code = &func->code;
  c->env = ExtendFrame(c->env, num_params);
  for (i = 0; i < num_params; i++) {
    FrameSet(c->env, i, lambda->params[i]);
  }

  EmitExtendEnv(num_params, c->code);
  EmitGetGlobal(regEnv, c->code);
  EmitHead(c->code);
  EmitSetGlobal(regTmp1, c->code);
  for (i = 0; i < num_params; i++) {
    EmitGetGlobal(regTmp1, c->code);
    EmitGetLocal(i, c->code);
    EmitStore(i*4, c->code);
  }

  if (!CompileExpr(lambda->body, c)) return false;
  EmitByte(EndOp, c->code);

  c->code = code;
  c->env = PopFrame(c->env);

  return FuncIdx(func, c->mod);
}

#define PageSize 65536
static void CompileUtilities(Module *mod)
{
  u8 **code;
  Func *alloc = AddFunc(AddType(1, 0, mod), 0, mod);
  Func *lookup = AddFunc(AddType(1, 1, mod), 2, mod);
  Func *makepair = AddFunc(AddType(2, 1, mod), 0, mod);
  Func *maketuple = AddFunc(AddType(1, 1, mod), 0, mod);

  code = &alloc->code;
  EmitGetGlobal(regFree, code);
  EmitGetLocal(0, code);
  EmitByte(I32Add, code);
  EmitSetGlobal(regFree, code);
  EmitByte(Block, code);
  EmitByte(EmptyBlock, code);
  EmitGetGlobal(regFree, code);
  EmitByte(MemSize, code);
  EmitByte(0, code);
  EmitConst(PageSize, code);
  EmitByte(I32Mul, code);
  EmitByte(I32LtS, code);
  EmitByte(BrIf, code);
  EmitInt(0, code);
  EmitConst(1, code);
  EmitByte(MemGrow, code);
  EmitByte(0, code);
  EmitByte(DropOp, code);
  EmitByte(EndOp, code);
  EmitByte(EndOp, code);
  allocFn = FuncIdx(alloc, mod);

  code = &lookup->code;
  EmitGetGlobal(regEnv, code);
  EmitSetLocal(1, code);
  EmitByte(Block, code);
  EmitByte(EmptyBlock, code);
  EmitByte(Loop, code);
  EmitByte(EmptyBlock, code);
  /* assert env != 0 */
  EmitByte(Block, code);
  EmitByte(EmptyBlock, code);
  EmitGetLocal(1, code);
  EmitConst(0, code);
  EmitByte(I32Ne, code);
  EmitByte(BrIf, code);
  EmitByte(0, code);
  EmitByte(Unreachable, code);
  EmitByte(EndOp, code);
  EmitGetLocal(0, code);
  EmitGetLocal(1, code);
  EmitLoad(0, code);
  EmitConst(4, code);
  EmitByte(I32Sub, code);
  EmitLoad(0, code);
  EmitTeeLocal(2, code);
  EmitByte(I32LtS, code);
  EmitByte(BrIf, code);
  EmitInt(1, code);
  EmitGetLocal(0, code);
  EmitGetLocal(2, code);
  EmitByte(I32Sub, code);
  EmitSetLocal(0, code);
  EmitGetLocal(1, code);
  EmitLoad(4, code);
  EmitSetLocal(1, code);
  EmitByte(Br, code);
  EmitInt(0, code);
  EmitByte(EndOp, code);
  EmitByte(EndOp, code);
  EmitGetLocal(1, code);
  EmitLoad(0, code);
  EmitGetLocal(0, code);
  EmitConst(4, code);
  EmitByte(I32Mul, code);
  EmitByte(I32Add, code);
  EmitLoad(0, code);
  EmitByte(EndOp, code);
  lookupFn = FuncIdx(lookup, mod);

  code = &makepair->code;
  EmitGetGlobal(regFree, code);
  EmitGetGlobal(regFree, code);
  EmitGetGlobal(regFree, code);
  EmitConst(2, code);
  EmitCall(allocFn, code);
  EmitGetLocal(1, code);
  EmitStore(0, code);
  EmitGetLocal(0, code);
  EmitStore(4, code);
  EmitByte(EndOp, code);
  makePairFn = FuncIdx(makepair, mod);

  code = &maketuple->code;
  /* store count */
  EmitGetGlobal(regFree, code);
  EmitGetLocal(0, code);
  EmitConst(4, code);
  EmitCall(allocFn, code);
  EmitStore(0, code);
  /* tuple pointer to return */
  EmitGetGlobal(regFree, code);
  /* allocate space for items */
  EmitGetLocal(0, code);
  EmitConst(4, code);
  EmitByte(I32Mul, code);
  EmitCall(allocFn, code);
  EmitByte(EndOp, code);
  makeTupleFn = FuncIdx(maketuple, mod);
}

static void ScanImports(Node *node, HashMap *imports, Module *mod)
{
  u32 i;
  Node **children = 0;

  if (IsTerminal(node)) return;

  if (node->type == moduleNode) {
    ModuleNode *module = (ModuleNode*)node;
    for (i = 0; i < VecCount(module->imports); i++) {
      HashMapSet(imports, module->imports[i]->alias, module->imports[i]->mod);
    }
    ScanImports((Node*)module->body, imports, mod);
    return;
  }

  if (node->type == callNode) {
    CallNode *call = (CallNode*)node;
    if (call->op->type == accessNode) {
      BinaryNode *access = (BinaryNode*)call->op;
      if (access->left->type == varNode && access->right->type == symbolNode) {
        TerminalNode *alias = (TerminalNode*)access->left;
        TerminalNode *name = (TerminalNode*)access->right;
        if (HashMapContains(imports, alias->value)) {
          u32 modname = HashMapGet(imports, alias->value);
          u32 type = AddType(VecCount(call->args), 1, mod);
          AddImport(SymbolName(modname), SymbolName(name->value), type, mod);
        }
      }
    }
  }

  switch (node->type) {
  case listNode:
  case tupleNode:
    children = ((ListNode*)node)->items;
    break;
  case negNode:
  case lenNode:
  case notNode:
    ScanImports(((UnaryNode*)node)->child, imports, mod);
    break;
  case pairNode:
  case accessNode:
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case ltEqNode:
  case inNode:
  case eqNode:
  case notEqNode:
  case gtNode:
  case gtEqNode:
  case andNode:
  case orNode:
    ScanImports(((BinaryNode*)node)->left, imports, mod);
    ScanImports(((BinaryNode*)node)->right, imports, mod);
    break;
  case ifNode:
    ScanImports(((IfNode*)node)->predicate, imports, mod);
    ScanImports(((IfNode*)node)->ifTrue, imports, mod);
    ScanImports(((IfNode*)node)->ifFalse, imports, mod);
    break;
  case doNode:
    children = ((DoNode*)node)->stmts;
    break;
  case callNode:
    children = ((CallNode*)node)->args;
    ScanImports(((CallNode*)node)->op, imports, mod);
    break;
  case lambdaNode:
    ScanImports(((LambdaNode*)node)->body, imports, mod);
    break;
  case defNode:
  case letNode:
    ScanImports(((LetNode*)node)->value, imports, mod);
    break;
  case moduleNode:
    ScanImports((Node*)((ModuleNode*)node)->body, imports, mod);
    break;
  default:
    break;

  }

  for (i = 0; i < VecCount(children); i++) {
    ScanImports(children[i], imports, mod);
  }
}

Result CompileModule(ModuleNode *ast)
{
  u32 i, j;
  Compiler *c = malloc(sizeof(Compiler));
  u32 *exports = ast->body->locals;
  u32 num_exports = VecCount(exports);
  Node **stmts = ast->body->stmts;
  HashMap imports = EmptyHashMap;
  Frame *env = 0;
  Func *start;

  c->mod = malloc(sizeof(Module));
  c->imports = &imports;

  InitModule(c->mod);
  c->mod->num_globals = 5; /* env, free, tmp1, tmp2, tmp3 */
  c->mod->filename = SymbolName(ast->filename);

  ScanImports((Node*)ast, &imports, c->mod);

  CompileUtilities(c->mod);

  start = AddFunc(AddType(0, 0, c->mod), 0, c->mod);
  env = ExtendFrame(env, VecCount(c->mod->imports));
  EmitExtendEnv(VecCount(c->mod->imports), &start->code);

  /* pre-define each func */
  if (num_exports > 0) {
    env = ExtendFrame(env, num_exports);
    EmitExtendEnv(num_exports, &start->code);
    for (i = 0; i < VecCount(stmts); i++) {
      LetNode *stmt = (LetNode*)stmts[i];
      if (stmt->type != defNode) continue;
      FrameSet(env, i, stmt->var);
    }

    /* compile each func */
    for (i = 0; i < VecCount(stmts); i++) {
      LetNode *stmt = (LetNode*)stmts[i];
      LambdaNode *lambda;
      i32 funcidx;

      if (stmt->type != defNode) continue;

      lambda = (LambdaNode*)stmt->value;
      funcidx = CompileFunc(lambda, c);
      if (funcidx < 0) return c->result;

      EmitGetGlobal(regEnv, &start->code);
      EmitHead(&start->code);

      EmitLambda(funcidx, &start->code);
      EmitStore(i*4, &start->code);

      AddExport(SymbolName(stmt->var), funcidx, c->mod);
    }
  }

  /* compile start func */
  c->code = &start->code;
  for (i = 0, j = 0; i < VecCount(stmts); i++) {
    Node *stmt = stmts[i];

    if (stmt->type == defNode) continue;

    if (!CompileExpr(stmt, c)) return c->result;
    EmitByte(DropOp, c->code);
  }
  EmitByte(EndOp, c->code);

  c->mod->start = FuncIdx(start, c->mod);

  return Ok(c->mod);
}
