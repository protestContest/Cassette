#include "compile.h"
#include "env.h"
#include "module.h"
#include "wasm.h"
#include <univ.h>

typedef struct {
  Module *mod;
  Env *env;
  u8 **code;
  HashMap *imports;
  Result result;
} Compiler;

#define regFree 0
#define regEnv  1
#define regTmp  2

static i32 allocFn = -1;
static i32 lookupFn = -1;
static i32 makePairFn = -1;
static i32 makeTupleFn = -1;
static i32 lenFn = -1;
static i32 accessFn = -1;

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
  i32 index = Lookup(expr->value, c->env);
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
    EmitSetGlobal(regTmp, c->code);
    EmitGetGlobal(regTmp, c->code);
    EmitGetGlobal(regTmp, c->code);

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
  if (VecCount(c->mod->imports) == 0) return -1;

  if (expr->type == accessNode) {
    BinaryNode* access = (BinaryNode*)expr;
    if (access->left->type == varNode && access->right->type == symbolNode) {
      TerminalNode *obj = (TerminalNode*)access->left;
      TerminalNode *name = (TerminalNode*)access->right;
      i32 frame = LookupFrame(obj->value, c->env);

      if (frame == 0 && HashMapContains(c->imports, obj->value)) {
        u32 modname = HashMapGet(c->imports, obj->value);
        return ImportIdx(SymbolName(modname), SymbolName(name->value), c->mod);
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

  for (i = 0; i < num_args; i++) {
    EmitGetGlobal(regEnv, c->code);
    if (!CompileExpr(expr->args[num_args-1-i], c)) return false;
    EmitSetGlobal(regTmp, c->code);
    EmitSetGlobal(regEnv, c->code);
    EmitGetGlobal(regTmp, c->code);
  }

  import = ImportCallIdx(expr->op, c);
  if (import >= 0) {
    EmitCall(import, c->code);
  } else {
    if (!CompileExpr(expr->op, c)) return false;
    EmitSetGlobal(regTmp, c->code);
    EmitGetGlobal(regTmp, c->code);
    EmitHead(c->code);
    EmitSetGlobal(regEnv, c->code);
    EmitGetGlobal(regTmp, c->code);
    EmitTail(c->code);
    EmitCallIndirect(AddType(num_args, 1, c->mod), c->code);
    EmitSetGlobal(regTmp, c->code);
    EmitSetGlobal(regEnv, c->code);
    EmitGetGlobal(regTmp, c->code);
  }

  return true;
}

static bool CompileDo(DoNode *expr, Compiler *c)
{
  u32 i;
  u32 num_stmts = VecCount(expr->stmts);
  u32 num_locals = VecCount(expr->locals);
  u32 num_defs = 0;

  if (num_locals > 0) {
    i32 export_env = (VecCount(c->mod->imports) > 0) ? 2 : 1;
    bool export_defs = EnvSize(c->env) == export_env;

    c->env = ExtendEnv(c->env, num_locals);
    EmitExtendEnv(num_locals, c->code);

    /* pre-define each def */
    for (i = 0; i < VecCount(expr->stmts); i++) {
      LetNode *stmt = (LetNode*)expr->stmts[i];
      if (stmt->type != defNode) continue;
      Define(stmt->var, i, c->env);
      num_defs++;
    }

    /* compile each def */
    for (i = 0; i < VecCount(expr->stmts); i++) {
      LetNode *stmt = (LetNode*)expr->stmts[i];
      LambdaNode *lambda;
      i32 funcidx;

      if (stmt->type != defNode) continue;

      lambda = (LambdaNode*)stmt->value;
      funcidx = CompileFunc(lambda, c);
      if (funcidx < 0) return false;

      EmitGetGlobal(regEnv, c->code);
      EmitHead(c->code);
      EmitLambda(funcidx, c->code);
      EmitStore(i*4, c->code);

      if (export_defs) AddExport(SymbolName(stmt->var), funcidx, c->mod);
    }
  }

  for (i = 0; i < num_stmts; i++) {
    Node *stmt = expr->stmts[i];
    bool is_last = i == num_stmts - 1;

    if (stmt->type == defNode) {
      if (is_last) EmitConst(0, c->code);
      continue;
    }

    /* save env */
    EmitGetGlobal(regEnv, c->code);

    if (stmt->type == letNode) {
      LetNode *let = (LetNode*)stmt;
      EmitGetGlobal(regEnv, c->code);
      EmitHead(c->code);
      if (!CompileExpr(let->value, c)) return false;
      EmitStore((num_defs + i) * 4, c->code);
      EmitSetGlobal(regEnv, c->code);
      if (is_last) {
        EmitConst(0, c->code);
      }
      Define(let->var, i, c->env);
    } else {
      if (!CompileExpr(expr->stmts[i], c)) return false;

      if (i < num_stmts - 1) {
        EmitByte(Drop, c->code);
        EmitSetGlobal(regEnv, c->code);
      } else {
        EmitSetGlobal(regTmp, c->code);
        EmitSetGlobal(regEnv, c->code);
        EmitGetGlobal(regTmp, c->code);
      }
    }
  }

  if (num_locals > 0) {
    c->env = PopEnv(c->env);
    EmitGetGlobal(regEnv, c->code);
    EmitTail(c->code);
    EmitSetGlobal(regEnv, c->code);
  }

  return true;
}

static bool CompileNil(Compiler *c)
{
  EmitConst(0, c->code);
  return true;
}

static bool CompileIf(IfNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->predicate, c)) return false;

  EmitByte(IfBlock, c->code);
  EmitByte(Int32, c->code);
  if (!CompileExpr(expr->ifTrue, c)) return false;

  EmitByte(ElseBlock, c->code);
  if (!CompileExpr(expr->ifFalse, c)) return false;

  EmitByte(End, c->code);
  return true;
}

static bool CompileAnd(BinaryNode *expr, Compiler *c)
{
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);
  if (!CompileExpr(expr->left, c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitGetGlobal(regTmp, c->code);
  EmitByte(I32EqZ, c->code);
  EmitByte(BrIf, c->code);
  EmitInt(0, c->code);

  if (!CompileExpr(expr->right, c)) return false;
  EmitSetGlobal(regTmp, c->code);

  EmitByte(End, c->code);
  EmitGetGlobal(regTmp, c->code);

  return true;
}

static bool CompileOr(BinaryNode *expr, Compiler *c)
{
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);

  if (!CompileExpr(expr->left, c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitGetGlobal(regTmp, c->code);

  EmitByte(I32EqZ, c->code);
  EmitByte(BrIf, c->code);
  EmitInt(0, c->code);

  EmitByte(Br, c->code);
  EmitInt(1, c->code);

  EmitByte(End, c->code);

  if (!CompileExpr(expr->right, c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitByte(End, c->code);

  EmitGetGlobal(regTmp, c->code);

  return true;
}

static bool CompileLength(UnaryNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->child, c)) return false;
  EmitCall(lenFn, c->code);
  return true;
}

static bool CompileAccess(BinaryNode *expr, Compiler *c)
{
  if (!CompileExpr(expr->left, c)) return false;
  if (!CompileExpr(expr->right, c)) return false;
  EmitCall(accessFn, c->code);
  return true;
}

static bool CompileString(TerminalNode *expr, Compiler *c)
{
  char *name = SymbolName(expr->value);
  u32 index = VecCount(c->mod->data);
  u32 len = strlen(name);
  GrowVec(c->mod->data, Align(len+4, 8));
  ((u32*)(c->mod->data + index))[0] = -len;
  Copy(name, c->mod->data + index + 4, len);
  EmitConst(index + 4, c->code);
  return true;
}

static bool CompileExpr(Node *expr, Compiler *c)
{
  printf("Compiling %s (%d)\n", NodeName(expr), expr->pos);

  switch (expr->type) {
  case nilNode:       return CompileNil(c);
  case varNode:       return CompileVar((TerminalNode*)expr, c);
  case intNode:       return CompileInt((TerminalNode*)expr, c);
  case symbolNode:    return CompileInt((TerminalNode*)expr, c);
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case ltEqNode:
  case eqNode:
  case notEqNode:
  case gtNode:
  case gtEqNode:
    return CompileBinOp((BinaryNode*)expr, c);
  case notNode:       return CompileNot((UnaryNode*)expr, c);
  case negNode:       return CompileNeg((UnaryNode*)expr, c);
  case pairNode:      return CompilePair((BinaryNode*)expr, c);
  case listNode:      return CompileList((ListNode*)expr, c);
  case tupleNode:     return CompileTuple((ListNode*)expr, c);
  case lambdaNode:    return CompileLambda((LambdaNode*)expr, c);
  case callNode:      return CompileCall((CallNode*)expr, c);
  case doNode:        return CompileDo((DoNode*)expr, c);
  case lenNode:       return CompileLength((UnaryNode*)expr, c);
  case accessNode:    return CompileAccess((BinaryNode*)expr, c);
  case stringNode:    return CompileString((TerminalNode*)expr, c);
  /*
  case floatNode: need type system
  case inNode: need type system
  */

  case idNode:
  case defNode:
  case letNode:
  case importNode:
  case moduleNode:
    return CompileError("Invalid expression", expr->pos, c);
  case ifNode:        return CompileIf((IfNode*)expr, c);
  case andNode:       return CompileAnd((BinaryNode*)expr, c);
  case orNode:        return CompileOr((BinaryNode*)expr, c);
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

  if (num_params > 0) {
    c->env = ExtendEnv(c->env, num_params);
    for (i = 0; i < num_params; i++) {
      Define(lambda->params[i], i, c->env);
    }
    EmitExtendEnv(num_params, c->code);

    EmitGetGlobal(regEnv, c->code);
    EmitHead(c->code);
    EmitSetGlobal(regTmp, c->code);
    for (i = 0; i < num_params; i++) {
      EmitGetGlobal(regTmp, c->code);
      EmitGetLocal(i, c->code);
      EmitStore(i*4, c->code);
    }
  }

  if (!CompileExpr(lambda->body, c)) return false;
  EmitByte(End, c->code);

  c->code = code;

  if (num_params > 0) {
    c->env = PopEnv(c->env);
  }

  return FuncIdx(func, c->mod);
}

#define PageSize 65536
static void CompileUtilities(Module *mod)
{
  u8 **code;
  Func *alloc = AddFunc(AddType(1, 0, mod), 0, mod);
  Func *makepair = AddFunc(AddType(2, 1, mod), 0, mod);
  Func *maketuple = AddFunc(AddType(1, 1, mod), 0, mod);
  Func *lookup = AddFunc(AddType(1, 1, mod), 2, mod);
  Func *length = AddFunc(AddType(1, 1, mod), 1, mod);
  Func *access = AddFunc(AddType(2, 1, mod), 0, mod);

  allocFn = FuncIdx(alloc, mod);
  makePairFn = FuncIdx(makepair, mod);
  makeTupleFn = FuncIdx(maketuple, mod);
  lookupFn = FuncIdx(lookup, mod);
  lenFn = FuncIdx(length, mod);
  accessFn = FuncIdx(access, mod);

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
  EmitByte(Drop, code);
  EmitByte(End, code);
  EmitByte(End, code);

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
  EmitByte(End, code);
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
  EmitByte(End, code);
  EmitByte(End, code);
  EmitGetLocal(1, code);
  EmitLoad(0, code);
  EmitGetLocal(0, code);
  EmitConst(4, code);
  EmitByte(I32Mul, code);
  EmitByte(I32Add, code);
  EmitLoad(0, code);
  EmitByte(End, code);

  code = &makepair->code;
  EmitGetGlobal(regFree, code);
  EmitGetGlobal(regFree, code);
  EmitGetGlobal(regFree, code);
  EmitConst(8, code);
  EmitCall(allocFn, code);
  EmitGetLocal(1, code);
  EmitStore(0, code);
  EmitGetLocal(0, code);
  EmitStore(4, code);
  EmitByte(End, code);

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
  EmitByte(End, code);

  code = &length->code;
  EmitGetLocal(0, code);
  EmitConst(8, code);
  EmitByte(I32RemS, code);
  EmitByte(IfBlock, code);
  EmitByte(Int32, code);
  /* tuple or binary */
  EmitGetLocal(0, code);
  EmitConst(4, code);
  EmitByte(I32Sub, code);
  EmitLoad(0, code);
  EmitTeeLocal(1, code);
  EmitConst(0, code);
  EmitByte(I32LtS, code);
  EmitByte(IfBlock, code);
  EmitByte(Int32, code);
  /* binary */
  EmitConst(0, code);
  EmitGetLocal(1, code);
  EmitByte(I32Sub, code);
  EmitByte(ElseBlock, code);
  /* tuple */
  EmitGetLocal(1, code);
  EmitByte(End, code);
  EmitByte(ElseBlock, code);
  /* pair */
  EmitConst(0, code);
  EmitSetLocal(1, code);
  EmitByte(Block, code);
  EmitByte(EmptyBlock, code);
  EmitByte(Loop, code);
  EmitByte(EmptyBlock, code);
  EmitGetLocal(0, code);
  EmitByte(I32EqZ, code);
  EmitByte(BrIf, code);
  EmitInt(1, code);
  EmitGetLocal(1, code);
  EmitConst(1, code);
  EmitByte(I32Add, code);
  EmitSetLocal(1, code);
  EmitGetLocal(0, code);
  EmitTail(code);
  EmitSetLocal(0, code);
  EmitByte(Br, code);
  EmitInt(0, code);
  EmitByte(End, code);
  EmitByte(End, code);
  EmitGetLocal(1, code);
  EmitByte(End, code);
  EmitByte(End, code);

  code = &access->code;
  EmitGetLocal(0, code);
  EmitConst(8, code);
  EmitByte(I32RemS, code);
  EmitByte(IfBlock, code);
  EmitByte(Int32, code);
    /* tuple or binary */
    EmitGetLocal(0, code);
    EmitConst(4, code);
    EmitByte(I32Sub, code);
    EmitLoad(0, code);
    EmitConst(0, code);
    EmitByte(I32GeS, code);
    EmitByte(IfBlock, code);
    EmitByte(Int32, code);
      /* tuple */
      EmitGetLocal(1, code);
      EmitConst(4, code);
      EmitByte(I32Mul, code);
      EmitGetLocal(0, code);
      EmitByte(I32Add, code);
      EmitLoad(0, code);
    EmitByte(ElseBlock, code);
      /* binary */
      EmitGetLocal(1, code);
      EmitGetLocal(0, code);
      EmitByte(I32Add, code);
      EmitLoadByte(0, code);
    EmitByte(End, code);
  EmitByte(ElseBlock, code);
    /* pair */
    EmitByte(Block, code);
    EmitByte(EmptyBlock, code);
    EmitByte(Loop, code);
    EmitByte(EmptyBlock, code);
      EmitGetLocal(1, code);
      EmitByte(I32EqZ, code);
      EmitByte(BrIf, code);
      EmitInt(1, code);
      EmitGetLocal(1, code);
      EmitConst(1, code);
      EmitByte(I32Sub, code);
      EmitSetLocal(1, code);
      EmitGetLocal(0, code);
      EmitTail(code);
      EmitSetLocal(0, code);
      EmitByte(Br, code);
      EmitInt(0, code);
    EmitByte(End, code);
    EmitByte(End, code);
    EmitGetLocal(0, code);
    EmitHead(code);
  EmitByte(End, code);
  EmitByte(End, code);
}

static void ScanImports(Node *node, HashMap *imports, Module *mod)
{
  u32 i;
  Node **children = 0;

  if (IsTerminal(node)) return;

  if (node->type == callNode) {
    CallNode *call = (CallNode*)node;
    if (call->op->type == accessNode) {
      BinaryNode *access = (BinaryNode*)call->op;
      if (access->left->type == varNode && access->right->type == symbolNode) {
        TerminalNode *alias = (TerminalNode*)access->left;
        TerminalNode *name = (TerminalNode*)access->right;
        if (HashMapContains(imports, alias->value)) {
          u32 modname = HashMapGet(imports, alias->value);
          if (!HasImport(modname, name->value, mod)) {
            u32 type = AddType(VecCount(call->args), 1, mod);
            AddImport(SymbolName(modname), SymbolName(name->value), type, mod);
          }
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
  Compiler *c = malloc(sizeof(Compiler));
  HashMap imports = EmptyHashMap;
  Func *start;

  c->result = Ok(0);
  c->env = 0;
  c->mod = malloc(sizeof(Module));
  InitModule(c->mod);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  VecPush(c->mod->data, 0);
  c->mod->filename = SymbolName(ast->filename);
  c->imports = &imports;

  /* define imports
  imports are detected by the compiler when compiling a function call, and these are all true:
  - the operator is an access node with two ID nodes
  - the accessed object ID (module name) is defined in the root env (imports)
  therefore, we only need to define the imported module names in the import environment
  */
  if (VecCount(ast->imports) > 0) {
    u32 i;
    c->env = ExtendEnv(c->env, VecCount(ast->imports));
    for (i = 0; i < VecCount(ast->imports); i++) {
      ImportNode *import = ast->imports[i];
      Define(import->alias, i, c->env);
      HashMapSet(&imports, import->alias, import->mod);
    }

    ScanImports(ast->body, c->imports, c->mod);
  }

  CompileUtilities(c->mod);

  start = AddFunc(AddType(0, 0, c->mod), 0, c->mod);
  c->code = &start->code;

  if (!CompileExpr(ast->body, c)) return c->result;
  EmitByte(Drop, c->code);
  EmitByte(End, c->code);

  VecPush(c->mod->globals, VecCount(c->mod->data));
  VecPush(c->mod->globals, 0);
  VecPush(c->mod->globals, 0);

  c->mod->start = FuncIdx(start, c->mod);

  return Ok(c->mod);
}
