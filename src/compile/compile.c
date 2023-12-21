#include "compile.h"
#include "parse.h"
#include "runtime/env.h"
#include "runtime/ops.h"
#include "project.h"
#include "runtime/primitives.h"

#define LinkNext false
#define LinkReturn true

static Result CompileImports(Val imports, Compiler *c);
static Result CompileExpr(Val node, bool linkage, Compiler *c);
static Result CompileBlock(Val stmts, Val assigns, bool linkage, Compiler *c);
static Result CompileDo(Val expr, bool linkage, Compiler *c);
static Result CompileCall(Val call, bool linkage, Compiler *c);
static Result CompileLambda(Val expr, bool linkage, Compiler *c);
static Result CompileIf(Val expr, bool linkage, Compiler *c);
static Result CompileAnd(Val expr, bool linkage, Compiler *c);
static Result CompileOr(Val expr, bool linkage, Compiler *c);
static Result CompileOp(OpCode op, Val expr, bool linkage, Compiler *c);
static Result CompileNotOp(OpCode op, Val expr, bool linkage, Compiler *c);
static Result CompileNegOp(OpCode op, Val args, bool linkage, Compiler *c);
static Result CompileList(Val items, bool linkage, Compiler *c);
static Result CompileTuple(Val items, bool linkage, Compiler *c);
static Result CompileMap(Val items, bool linkage, Compiler *c);
static Result CompileString(Val sym, bool linkage, Compiler *c);
static Result CompileVar(Val id, bool linkage, Compiler *c);
static Result CompileConst(Val value, bool linkage, Compiler *c);

static void CompileLinkage(bool linkage, Compiler *c);
static Result CompileError(char *message, Compiler *c);
#define CompileOk()  OkResult(Nil)

void InitCompiler(Compiler *c, Mem *mem, SymbolTable *symbols, HashMap *modules, Chunk *chunk)
{
  c->pos = 0;
  c->env = Nil;
  c->filename = 0;
  c->mem = mem;
  c->symbols = symbols;
  c->modules = modules;
  c->chunk = chunk;
}

Result CompileInitialEnv(u32 num_modules, Compiler *c)
{
  PrimitiveModuleDef *primitives = GetPrimitives();
  u32 num_primitives = NumPrimitives();
  u32 frame_size = num_primitives + num_modules;
  Val env;
  u32 mod;

  c->filename = "*startup*";
  c->pos = 0;
  BeginChunkFile(Sym(c->filename, &c->chunk->symbols), c->chunk);

  PushConst(IntVal(frame_size), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);
  PushByte(OpExtend, c->pos, c->chunk);
  env = ExtendEnv(Nil, MakeTuple(frame_size, c->mem), c->mem);

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

    Define(primitives[mod].module, num_modules + mod, env, c->mem);
  }

  EndChunkFile(c->chunk);

  return OkResult(env);
}

Result CompileScript(Val module, Compiler *c)
{
  Result result;
  Val imports = ModuleImports(module, c->mem);

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module, c->mem), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  BeginChunkFile(ModuleFile(module, c->mem), c->chunk);

  /* compile imports */
  if (imports != Nil) {
    u32 num_imports = CountExports(imports, c->modules, c->mem);

    /* extend env for imports */
    PushConst(IntVal(num_imports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendEnv(c->env, MakeTuple(num_imports, c->mem), c->mem);

    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  result = CompileDo(Pair(ModuleExports(module, c->mem), ModuleBody(module, c->mem), c->mem), LinkNext, c);
  if (!result.ok) return result;

  /* discard import frame */
  if (imports != Nil) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  }

  EndChunkFile(c->chunk);

  return CompileOk();
}

Result CompileModule(Val module, u32 mod_num, Compiler *c)
{
  Result result;
  Val imports = ModuleImports(module, c->mem);
  Val exports = ModuleExports(module, c->mem);
  u32 num_assigns = ListLength(exports, c->mem);
  u32 jump, link;

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module, c->mem), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  BeginChunkFile(ModuleFile(module, c->mem), c->chunk);

  /* create lambda for module */
  PushConst(IntVal(0), c->pos, c->chunk);
  link = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpLambda, c->pos, c->chunk);
  jump = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpJump, c->pos, c->chunk);
  PatchConst(c->chunk, link);

  /* compile imports */
  if (imports != Nil) {
    u32 num_imports = CountExports(imports, c->modules, c->mem);

    /* extend env for imports */
    PushConst(IntVal(num_imports), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendEnv(c->env, MakeTuple(num_imports, c->mem), c->mem);

    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  /* extend env for assigns */
  if (num_assigns > 0) {
    PushConst(IntVal(num_assigns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, c->mem), c->mem);
  }

  result = CompileBlock(ModuleBody(module, c->mem), ModuleExports(module, c->mem), LinkNext, c);
  if (!result.ok) return result;

  /* discard last statement result */
  PushByte(OpPop, c->pos, c->chunk);

  /* export assigned values */
  if (num_assigns > 0) {
    u32 i = 0;
    PushByte(OpExport, c->pos, c->chunk);

    /* build tuple of keys */
    PushConst(IntVal(num_assigns), c->pos, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);

    while (exports != Nil) {
      Val export = Head(exports, c->mem);
      Sym(SymbolName(export, c->symbols), &c->chunk->symbols);
      PushConst(export, c->pos, c->chunk);
      PushConst(IntVal(i), c->pos, c->chunk);
      PushByte(OpSet, c->pos, c->chunk);
      exports = Tail(exports, c->mem);
      i++;
    }
    PushByte(OpPair, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
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
  if (imports != Nil) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
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

static Result CompileImports(Val imports, Compiler *c)
{
  u32 num_imported = 0;

  while (imports != Nil) {
    Val node = Head(imports, c->mem);
    Val import = NodeExpr(node, c->mem);
    Val import_name = Head(import, c->mem);
    Val alias = Tail(import, c->mem);
    Val import_mod;
    Val imported_vals;
    i32 import_def;
    u32 link;

    c->pos = NodePos(node, c->mem);

    if (!HashMapContains(c->modules, import_name)) return CompileError("Undefined module", c);

    import_mod = HashMapGet(c->modules, import_name);
    imported_vals = ModuleExports(import_mod, c->mem);

    import_def = FindModule(import_name, c->env, c->mem);
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
      /* define each exported value */
      u32 i = 0;

      PushByte(OpUnpair, c->pos, c->chunk);
      PushByte(OpPop, c->pos, c->chunk);

      while (imported_vals != Nil) {
        Val imported_val = Head(imported_vals, c->mem);

        if (Tail(imported_vals, c->mem) != Nil) {
          /* keep tuple for future iterations */
          PushByte(OpDup, c->pos, c->chunk);
        }
        PushConst(IntVal(i), c->pos, c->chunk);
        PushByte(OpGet, c->pos, c->chunk);
        PushConst(IntVal(num_imported+i), c->pos, c->chunk);
        PushByte(OpDefine, c->pos, c->chunk);
        Define(imported_val, num_imported + i, c->env, c->mem);

        imported_vals = Tail(imported_vals, c->mem);
        i++;
      }
      num_imported += i;
    } else {
      /* import into a map */
      PushByte(OpUnpair, c->pos, c->chunk);
      PushByte(OpMap, c->pos, c->chunk);

      PushConst(IntVal(num_imported), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      Define(alias, num_imported, c->env, c->mem);
      num_imported++;
    }

    imports = Tail(imports, c->mem);
  }

  return CompileOk();
}

static Result CompileExpr(Val node, bool linkage, Compiler *c)
{
  Val tag = TupleGet(node, 0, c->mem);
  Val expr = TupleGet(node, 2, c->mem);
  c->pos = RawInt(TupleGet(node, 1, c->mem));

  switch (tag) {
  case SymID:             return CompileVar(expr, linkage, c);
  case SymBangEqual:      return CompileNotOp(OpEq, expr, linkage, c);
  case SymString:         return CompileString(expr, linkage, c);
  case SymHash:           return CompileOp(OpLen, expr, linkage, c);
  case SymPercent:        return CompileOp(OpRem, expr, linkage, c);
  case SymAmpersand:      return CompileOp(OpBitAnd, expr, linkage, c);
  case SymLParen:         return CompileCall(expr, linkage, c);
  case SymStar:           return CompileOp(OpMul, expr, linkage, c);
  case SymPlus:           return CompileOp(OpAdd, expr, linkage, c);
  case SymMinus:
    if (ListLength(expr, c->mem) == 1) return CompileOp(OpNeg, expr, linkage, c);
    else return CompileOp(OpSub, expr, linkage, c);
  case SymArrow:          return CompileLambda(expr, linkage, c);
  case SymSlash:          return CompileOp(OpDiv, expr, linkage, c);
  case SymNum:            return CompileConst(expr, linkage, c);
  case SymColon:          return CompileConst(expr, linkage, c);
  case SymLess:           return CompileOp(OpLt, expr, linkage, c);
  case SymLessLess:       return CompileOp(OpShift, expr, linkage, c);
  case SymLessEqual:      return CompileNotOp(OpGt, expr, linkage, c);
  case SymLessGreater:    return CompileOp(OpCat, expr, linkage, c);
  case SymEqualEqual:     return CompileOp(OpEq, expr, linkage, c);
  case SymGreater:        return CompileOp(OpGt, expr, linkage, c);
  case SymGreaterEqual:   return CompileNotOp(OpLt, expr, linkage, c);
  case SymGreaterGreater: return CompileNegOp(OpShift, expr, linkage, c);
  case SymLBracket:       return CompileList(expr, linkage, c);
  case SymCaret:          return CompileOp(OpBitOr, expr, linkage, c);
  case SymAnd:            return CompileAnd(expr, linkage, c);
  case SymDo:             return CompileDo(expr, linkage, c);
  case SymIf:             return CompileIf(expr, linkage, c);
  case SymIn:             return CompileOp(OpIn, expr, linkage, c);
  case SymNil:            return CompileConst(Nil, linkage, c);
  case SymNot:            return CompileOp(OpNot, expr, linkage, c);
  case SymOr:             return CompileOr(expr, linkage, c);
  case SymLBrace:         return CompileTuple(expr, linkage, c);
  case SymBar:            return CompileOp(OpPair, expr, linkage, c);
  case SymRBrace:         return CompileMap(expr, linkage, c);
  case SymTilde:          return CompileOp(OpBitNot, expr, linkage, c);
  default:                return CompileError("Unknown expression", c);
  }
}

static Result CompileBlock(Val stmts, Val assigns, bool linkage, Compiler *c)
{
  u32 num_assigned = 0;
  Val def_stmts = stmts;

  /* pre-define all def statements */
  while (def_stmts != Nil) {
    Val stmt = Head(def_stmts, c->mem);
    Val var;
    def_stmts = Tail(def_stmts, c->mem);

    /* keep the assignment slots aligned */
    if (NodeType(stmt, c->mem) == SymLet) {
      num_assigned++;
      continue;
    }
    if (NodeType(stmt, c->mem) != SymDef) continue;

    var = Head(NodeExpr(stmt, c->mem), c->mem);
    Define(var, num_assigned, c->env, c->mem);
    num_assigned++;
  }

  num_assigned = 0;
  while (stmts != Nil) {
    Result result;
    Val stmt = Head(stmts, c->mem);
    bool is_last = Tail(stmts, c->mem) == Nil;
    bool stmt_linkage = is_last ? linkage : LinkNext;

    if (NodeType(stmt, c->mem) == SymLet) {
      Val var = Head(NodeExpr(stmt, c->mem), c->mem);
      Val value = Tail(NodeExpr(stmt, c->mem), c->mem);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushConst(IntVal(num_assigned), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      Define(var, num_assigned, c->env, c->mem);
      num_assigned++;

      if (is_last) {
        /* the last statement must produce a result */
        PushConst(Nil, c->pos, c->chunk);
      }
    } else if (NodeType(stmt, c->mem) == SymDef) {
      Val value = Tail(NodeExpr(stmt, c->mem), c->mem);

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

    stmts = Tail(stmts, c->mem);
  }

  return CompileOk();
}

static Result CompileDo(Val expr, bool linkage, Compiler *c)
{
  Result result;
  Val assigns = Head(expr, c->mem);
  Val stmts = Tail(expr, c->mem);

  u32 num_assigns = ListLength(assigns, c->mem);

  if (num_assigns > 0) {
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, c->mem), c->mem);
    PushConst(IntVal(num_assigns), 0, c->chunk);
    PushByte(OpTuple, c->pos, c->chunk);
    PushByte(OpExtend, c->pos, c->chunk);
  }

  result = CompileBlock(stmts, assigns, linkage, c);
  if (!result.ok) return result;

  /* discard frame for assigns */
  if (num_assigns > 0) {
    PushByte(OpExport, c->pos, c->chunk);
    PushByte(OpPop, c->pos, c->chunk);
    c->env = Tail(c->env, c->mem);
  }

  return CompileOk();
}

static Result CompileCall(Val call, bool linkage, Compiler *c)
{
  Val op = Head(call, c->mem);
  Val args = Tail(call, c->mem);
  u32 num_args = 0, patch;
  Result result;

  if (linkage != LinkReturn) {
    patch = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpLink, c->pos, c->chunk);
  }

  while (args != Nil) {
    Val arg = Head(args, c->mem);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
    num_args++;
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

static Result CompileLambda(Val expr, bool linkage, Compiler *c)
{
  Val params = Head(expr, c->mem);
  Val body = Tail(expr, c->mem);
  Result result;
  u32 jump, link, i;
  u32 num_params = ListLength(params, c->mem);

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

    c->env = ExtendEnv(c->env, MakeTuple(num_params, c->mem), c->mem);
    for (i = 0; i < num_params; i++) {
      Val param = Head(params, c->mem);
      Define(param, i, c->env, c->mem);
      PushConst(IntVal(num_params - i - 1), c->pos, c->chunk);
      PushByte(OpDefine, c->pos, c->chunk);
      params = Tail(params, c->mem);
    }
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  PatchConst(c->chunk, jump);

  CompileLinkage(linkage, c);

  if (num_params > 0) c->env = Tail(c->env, c->mem);

  return CompileOk();
}

static Result CompileIf(Val expr, bool linkage, Compiler *c)
{
  Val pred = Head(expr, c->mem);
  Val cons = Head(Tail(expr, c->mem), c->mem);
  Val alt = Head(Tail(Tail(expr, c->mem), c->mem), c->mem);
  Result result;
  u32 branch, jump;

  result = CompileExpr(pred, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->pos, c->chunk);
  PushByte(OpBranch, c->pos, c->chunk);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(alt, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->pos, c->chunk);
    PushByte(OpJump, c->pos, c->chunk);
  }

  PatchConst(c->chunk, branch);

  PushByte(OpPop, c->pos, c->chunk);
  result = CompileExpr(cons, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump);
  }

  return CompileOk();
}

static Result CompileAnd(Val expr, bool linkage, Compiler *c)
{
  Val a = Head(expr, c->mem);
  Val b = Head(Tail(expr, c->mem), c->mem);
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

static Result CompileOr(Val expr, bool linkage, Compiler *c)
{
  Val a = Head(expr, c->mem);
  Val b = Head(Tail(expr, c->mem), c->mem);
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

static Result CompileOp(OpCode op, Val args, bool linkage, Compiler *c)
{
  u32 pos = c->pos;
  while (args != Nil) {
    Result result = CompileExpr(Head(args, c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
  }
  PushByte(op, pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileNotOp(OpCode op, Val args, bool linkage, Compiler *c)
{
  while (args != Nil) {
    Result result = CompileExpr(Head(args, c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
  }
  PushByte(op, c->pos, c->chunk);
  PushByte(OpNot, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileNegOp(OpCode op, Val args, bool linkage, Compiler *c)
{
  while (args != Nil) {
    Result result = CompileExpr(Head(args, c->mem), LinkNext, c);
    if (!result.ok) return result;
    args = Tail(args, c->mem);
  }
  PushByte(OpNeg, c->pos, c->chunk);
  PushByte(op, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileList(Val items, bool linkage, Compiler *c)
{
  PushConst(Nil, c->pos, c->chunk);

  while (items != Nil) {
    Val item = Head(items, c->mem);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->pos, c->chunk);
    items = Tail(items, c->mem);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileTuple(Val items, bool linkage, Compiler *c)
{
  u32 i = 0, num_items = ListLength(items, c->mem);

  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  while (items != Nil) {
    Val item = Head(items, c->mem);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);
    i++;
    items = Tail(items, c->mem);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileMap(Val items, bool linkage, Compiler *c)
{
  u32 i = 0, num_items = ListLength(items, c->mem);
  Val keys = items;

  /* create tuple for values */
  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  while (items != Nil) {
    Val item = Head(items, c->mem);
    Val value = Tail(item, c->mem);

    Result result = CompileExpr(value, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);
    i++;
    items = Tail(items, c->mem);
  }

  /* create tuple for keys */
  PushConst(IntVal(num_items), c->pos, c->chunk);
  PushByte(OpTuple, c->pos, c->chunk);

  i = 0;
  while (keys != Nil) {
    Val item = Head(keys, c->mem);
    Val key = Head(item, c->mem);

    Result result = CompileExpr(key, LinkNext, c);
    if (!result.ok) return result;

    PushConst(IntVal(i), c->pos, c->chunk);
    PushByte(OpSet, c->pos, c->chunk);

    i++;
    keys = Tail(keys, c->mem);
  }

  /* create map from tuples */
  PushByte(OpMap, c->pos, c->chunk);

  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileString(Val sym, bool linkage, Compiler *c)
{
  Sym(SymbolName(sym, c->symbols), &c->chunk->symbols);
  PushConst(sym, c->pos, c->chunk);
  PushByte(OpStr, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileVar(Val id, bool linkage, Compiler *c)
{
  i32 def = FindDefinition(id, c->env, c->mem);
  if (def == -1) return CompileError("Undefined variable", c);

  PushConst(IntVal(def), c->pos, c->chunk);
  PushByte(OpLookup, c->pos, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

static Result CompileConst(Val value, bool linkage, Compiler *c)
{
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
