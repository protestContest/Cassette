#include "compile.h"
#include "env.h"
#include "module.h"
#include "ops.h"
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

static i32 CompileFunc(Node *lambda, Compiler *c);
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

static bool CompileVar(Node *expr, Compiler *c)
{
  i32 index = Lookup(expr->value, c->env);
  if (index < 0) return CompileError("Undefined variable", expr->pos, c);

  EmitConst(index, c->code);
  EmitCall(lookupFn, c->code);

  return true;
}

static bool CompileInt(Node *expr, Compiler *c)
{
  EmitConst(expr->value, c->code);
  return true;
}

static OpCode NodeOp(Node *node)
{
  switch (node->type) {
  case addNode:     return I32Add;
  case subNode:     return I32Sub;
  case mulNode:     return I32Mul;
  case divNode:     return I32DivS;
  case remNode:     return I32RemS;
  case ltNode:      return I32LtS;
  case gtNode:      return I32GtS;
  case eqNode:      return I32Eq;
  default:          return Unreachable;
  }
}

static bool CompileNeg(Node *expr, Compiler *c)
{
  EmitConst(0, c->code);
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitByte(I32Sub, c->code);
  return true;
}

static bool CompileNot(Node *expr, Compiler *c)
{
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitByte(I32EqZ, c->code);
  return true;
}

static bool CompileBinOp(Node *expr, Compiler *c)
{
  if (!CompileExpr(expr->children[1], c)) return false;
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitByte(NodeOp(expr), c->code);
  return true;
}

static bool CompilePair(Node *expr, Compiler *c)
{
  if (!CompileExpr(expr->children[1], c)) return false;
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitCall(makePairFn, c->code);
  return true;
}

static bool CompileList(Node *expr, Compiler *c)
{
  u32 i;
  u32 len = NodeCount(expr);
  EmitConst(0, c->code);
  for (i = 0; i < len; i++) {
    if (!CompileExpr(expr->children[len-1-i], c)) return false;
    EmitCall(makePairFn, c->code);
  }
  return true;
}

static bool CompileTuple(Node *expr, Compiler *c)
{
  u32 i;
  u32 num_items = NodeCount(expr);

  EmitConst(num_items, c->code);
  EmitCall(makeTupleFn, c->code);

  for (i = 0; i < num_items; i++) {
    EmitSetGlobal(regTmp, c->code);
    EmitGetGlobal(regTmp, c->code);
    EmitGetGlobal(regTmp, c->code);

    if (!CompileExpr(expr->children[i], c)) return false;

    EmitStore(i*4, c->code);
  }

  return true;
}

static bool CompileLambda(Node *expr, Compiler *c)
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
    if (expr->children[0]->type == idNode && expr->children[1]->type == idNode) {
      Node *obj = expr->children[0];
      Node *name = expr->children[1];
      i32 frame = LookupFrame(obj->value, c->env);

      if (frame == 0 && HashMapContains(c->imports, obj->value)) {
        u32 modname = HashMapGet(c->imports, obj->value);
        return ImportIdx(SymbolName(modname), SymbolName(name->value), c->mod);
      }
    }
  }
  return -1;
}

static bool CompileCall(Node *expr, Compiler *c)
{
  u32 i;
  i32 import;
  u32 num_args = NodeCount(expr) - 1;
  Node *op = expr->children[0];

  for (i = 0; i < num_args; i++) {
    EmitGetGlobal(regEnv, c->code);
    if (!CompileExpr(expr->children[num_args-i], c)) return false;
    EmitSetGlobal(regTmp, c->code);
    EmitSetGlobal(regEnv, c->code);
    EmitGetGlobal(regTmp, c->code);
  }

  import = ImportCallIdx(op, c);
  if (import >= 0) {
    EmitCall(import, c->code);
  } else {
    if (!CompileExpr(op, c)) return false;
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

static bool CompileDo(Node *expr, Compiler *c)
{
  u32 i;
  Node *locals = DoNodeLocals(expr);
  Node *stmts = DoNodeStmts(expr);
  u32 num_locals = NodeCount(locals);
  u32 num_stmts = NodeCount(stmts);
  u32 num_defs = 0;

  if (num_locals > 0) {
    i32 export_env = (VecCount(c->mod->imports) > 0) ? 2 : 1;
    bool export_defs = EnvSize(c->env) == export_env;

    c->env = ExtendEnv(c->env, num_locals);
    EmitExtendEnv(num_locals, c->code);

    /* pre-define each def */
    for (i = 0; i < num_stmts; i++) {
      Node *stmt = stmts->children[i];
      if (stmt->type != defNode) continue;
      Define(AssignNodeVar(stmt)->value, i, c->env);
      num_defs++;
    }

    /* compile each def */
    for (i = 0; i < num_stmts; i++) {
      Node *stmt = stmts->children[i];
      i32 funcidx;

      if (stmt->type != defNode) continue;

      funcidx = CompileFunc(AssignNodeValue(stmt), c);
      if (funcidx < 0) return false;

      EmitGetGlobal(regEnv, c->code);
      EmitHead(c->code);
      EmitLambda(funcidx, c->code);
      EmitStore(i*4, c->code);

      if (export_defs) AddExport(SymbolName(AssignNodeVar(stmt)->value), funcidx, c->mod);
    }
  }

  for (i = 0; i < num_stmts; i++) {
    Node *stmt = stmts->children[i];
    bool is_last = i == num_stmts - 1;

    if (stmt->type == defNode) {
      if (is_last) EmitConst(0, c->code);
      continue;
    }

    /* save env */
    EmitGetGlobal(regEnv, c->code);

    if (stmt->type == letNode) {
      Node *let = stmt;
      EmitGetGlobal(regEnv, c->code);
      EmitHead(c->code);
      if (!CompileExpr(AssignNodeValue(let), c)) return false;
      EmitStore((num_defs + i) * 4, c->code);
      EmitSetGlobal(regEnv, c->code);
      if (is_last) {
        EmitConst(0, c->code);
      }
      Define(AssignNodeVar(let)->value, i, c->env);
    } else {
      if (!CompileExpr(stmts->children[i], c)) return false;

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

static bool CompileIf(Node *expr, Compiler *c)
{
  if (!CompileExpr(IfNodePredicate(expr), c)) return false;

  EmitByte(IfBlock, c->code);
  EmitByte(Int32, c->code);
  if (!CompileExpr(IfNodeConsequent(expr), c)) return false;

  EmitByte(ElseBlock, c->code);
  if (!CompileExpr(IfNodeAlternative(expr), c)) return false;

  EmitByte(End, c->code);
  return true;
}

static bool CompileAnd(Node *expr, Compiler *c)
{
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitGetGlobal(regTmp, c->code);
  EmitByte(I32EqZ, c->code);
  EmitByte(BrIf, c->code);
  EmitInt(0, c->code);

  if (!CompileExpr(expr->children[1], c)) return false;
  EmitSetGlobal(regTmp, c->code);

  EmitByte(End, c->code);
  EmitGetGlobal(regTmp, c->code);

  return true;
}

static bool CompileOr(Node *expr, Compiler *c)
{
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);
  EmitByte(Block, c->code);
  EmitByte(EmptyBlock, c->code);

  if (!CompileExpr(expr->children[0], c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitGetGlobal(regTmp, c->code);

  EmitByte(I32EqZ, c->code);
  EmitByte(BrIf, c->code);
  EmitInt(0, c->code);

  EmitByte(Br, c->code);
  EmitInt(1, c->code);

  EmitByte(End, c->code);

  if (!CompileExpr(expr->children[1], c)) return false;
  EmitSetGlobal(regTmp, c->code);
  EmitByte(End, c->code);

  EmitGetGlobal(regTmp, c->code);

  return true;
}

static bool CompileLength(Node *expr, Compiler *c)
{
  if (!CompileExpr(expr->children[0], c)) return false;
  EmitCall(lenFn, c->code);
  return true;
}

static bool CompileAccess(Node *expr, Compiler *c)
{
  if (!CompileExpr(expr->children[0], c)) return false;
  if (!CompileExpr(expr->children[1], c)) return false;
  EmitCall(accessFn, c->code);
  return true;
}

static bool CompileString(Node *expr, Compiler *c)
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
  case idNode:        return CompileVar(expr, c);
  case intNode:       return CompileInt(expr, c);
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case eqNode:
  case gtNode:        return CompileBinOp(expr, c);
  case notNode:       return CompileNot(expr, c);
  case negNode:       return CompileNeg(expr, c);
  case pairNode:      return CompilePair(expr, c);
  case listNode:      return CompileList(expr, c);
  case tupleNode:     return CompileTuple(expr, c);
  case lambdaNode:    return CompileLambda(expr, c);
  case callNode:      return CompileCall(expr, c);
  case doNode:        return CompileDo(expr, c);
  case lenNode:       return CompileLength(expr, c);
  case accessNode:    return CompileAccess(expr, c);
  case stringNode:    return CompileString(expr, c);
  /*
  case floatNode: need type system
  case inNode: need type system
  */

  case defNode:
  case letNode:
  case importNode:
  case moduleNode:
    return CompileError("Invalid expression", expr->pos, c);
  case ifNode:        return CompileIf(expr, c);
  case andNode:       return CompileAnd(expr, c);
  case orNode:        return CompileOr(expr, c);
  default:            return CompileError("Unknown expr type", expr->pos, c);
  }
}

static i32 CompileFunc(Node *lambda, Compiler *c)
{
  u32 i;
  Node *params = LambdaNodeParams(lambda);
  u32 num_params = NodeCount(params);
  u32 type = AddType(num_params, 1, c->mod);
  Func *func = AddFunc(type, 0, c->mod);
  u8 **code = c->code;

  c->code = &func->code;

  if (num_params > 0) {
    c->env = ExtendEnv(c->env, num_params);
    for (i = 0; i < num_params; i++) {
      Define(params->children[i]->value, i, c->env);
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

  if (!CompileExpr(LambdaNodeBody(lambda), c)) return false;
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

  if (IsTerminal(node)) return;

  if (node->type == callNode) {
    if (node->children[0]->type == accessNode) {
      Node *access = node->children[0];
      if (access->children[0]->type == idNode && access->children[1]->type == idNode) {
        Node *alias = access->children[0];
        Node *name = access->children[1];
        if (HashMapContains(imports, alias->value)) {
          u32 modname = HashMapGet(imports, alias->value);
          if (!HasImport(modname, name->value, mod)) {
            u32 type = AddType(NodeCount(node) - 1, 1, mod);
            AddImport(SymbolName(modname), SymbolName(name->value), type, mod);
          }
        }
      }
    }
  }

  for (i = 0; i < NodeCount(node); i++) {
    ScanImports(node->children[i], imports, mod);
  }
}

Result CompileModule(Node *ast)
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
  c->mod->filename = SymbolName(ModuleNodeFilename(ast)->value);
  c->imports = &imports;

  /* define imports
  imports are detected by the compiler when compiling a function call, and these are all true:
  - the operator is an access node with two ID nodes
  - the accessed object ID (module name) is defined in the root env (imports)
  therefore, we only need to define the imported module names in the import environment
  */
  if (NodeCount(ModuleNodeImports(ast)) > 0) {
    u32 i;
    c->env = ExtendEnv(c->env, NodeCount(ModuleNodeImports(ast)));
    for (i = 0; i < NodeCount(ModuleNodeImports(ast)); i++) {
      Node *import = ModuleNodeImports(ast)->children[i];
      Define(ImportNodeAlias(import)->value, i, c->env);
      HashMapSet(&imports, ImportNodeAlias(import)->value, ImportNodeMod(import)->value);
    }

    ScanImports(ModuleNodeBody(ast), c->imports, c->mod);
  }

  CompileUtilities(c->mod);

  start = AddFunc(AddType(0, 0, c->mod), 0, c->mod);
  c->code = &start->code;

  if (!CompileExpr(ModuleNodeBody(ast), c)) return c->result;
  EmitByte(Drop, c->code);
  EmitByte(End, c->code);

  VecPush(c->mod->globals, VecCount(c->mod->data));
  VecPush(c->mod->globals, 0);
  VecPush(c->mod->globals, 0);

  c->mod->start = FuncIdx(start, c->mod);

  return Ok(c->mod);
}
