#include "parse.h"
#include "lex.h"
#include "mem.h"
#include "node.h"
#include "univ/hashmap.h"
#include "univ/str.h"
#include "univ/symbol.h"

typedef struct {
  char *text;
  Token token;
} Parser;

typedef ASTNode* (*ParsePrefix)(Parser *p);
typedef ASTNode* (*ParseInfix)(ASTNode *expr, Parser *p);
typedef enum {
  precNone, precExpr, precLogic, precEqual, precPair, precJoin,
  precCompare, precShift, precSum, precProduct, precCall, precQualify
} Precedence;
typedef struct {
  ParsePrefix prefix;
  ParseInfix infix;
  Precedence prec;
} ParseRule;

#define Adv(p) \
  (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))
#define Lexeme(token, p)    SymbolFrom((p)->text + (token).pos, (token).length)
#define ParseFail(n,r)      (FreeNode(n), (r))
#define Spacing(p)          MatchToken(spaceToken, p)
#define MakeTerminal(type, value, p) \
  NewNode(type, (p)->token.pos, (p)->token.pos + (p)->token.length, value)
#define MakeNode(type, p)   MakeTerminal(type, 0, p)
#define NilNode(p) \
  NewNode(constNode, (p)->token.pos, (p)->token.pos, 0)
#define EmptyNode(p) \
  NewNode(tupleNode, (p)->token.pos, (p)->token.pos, 0)
#define Expected(s, n, p) \
  ParseFail(n, ParseError("Expected \"" s "\"", p))

static void InitParser(Parser *p, char *text)
{
  p->text = text;
  p->token.pos = 0;
  p->token.length = 0;
  Adv(p);
}

static ASTNode *ParseError(char *msg, Parser *p)
{
  char *error = NewString("Parse error: ^");
  error = FormatString(error, msg);
  return MakeTerminal(errorNode, Symbol(error), p);
}

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

static ASTNode *ParseModuleName(Parser *p);
static ASTNode *ParseImports(Parser *p);
static ASTNode *ParseImportList(Parser *p);
static ASTNode *ParseImport(Parser *p);
static ASTNode *ParseAlias(ASTNode *name, Parser *p);
static ASTNode *ParseFnList(Parser *p);
static ASTNode *TransformImports(ASTNode *node, ASTNode *body, Parser *p);
static ASTNode *ParseExports(Parser *p);
static ASTNode *ParseStmts(Parser *p);
static ASTNode *ParseDef(Parser *p);
static ASTNode *ParseParams(Parser *p);
static ASTNode *ParseDefGuard(Parser *p);
static ASTNode *ParseRecord(Parser *p);
#define ParseExpr(p)  ParsePrec(precExpr, p)
static ASTNode *ParsePrec(Precedence prec, Parser *p);
static ASTNode *ParseOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNegOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNotOp(ASTNode *expr, Parser *p);
static ASTNode *ParsePair(ASTNode *expr, Parser *p);
static ASTNode *ParseCall(ASTNode *expr, Parser *p);
static ASTNode *ParseTrap(Parser *p);
static ASTNode *ParseAccess(ASTNode *expr, Parser *p);
static ASTNode *ParseUnary(Parser *p);
static ASTNode *ParseLet(Parser *p);
static ASTNode *ParseAssigns(u32 *num_assigns, Parser *p);
static ASTNode *ParseAssign(Parser *p);

static ASTNode *ParseItems(Parser *p);
static ASTNode *ParseIDList(Parser *p);
static ASTNode *ParseID(Parser *p);
static void VSpacing(Parser *p);

static ASTNode *ParseLambda(Parser *p);
static ASTNode *ParseGroup(Parser *p);
static ASTNode *ParseDo(Parser *p);
static ASTNode *ParseIf(Parser *p);
static ASTNode *ParseCond(Parser *p);
static ASTNode *ParseClauses(Parser *p);
static ASTNode *ParseList(Parser *p);
static ASTNode *ParseListTail(Parser *p);
static ASTNode *ParseTuple(Parser *p);
static ASTNode *ParseNum(Parser *p);
static ASTNode *ParseByte(Parser *p);
static ASTNode *ParseHex(Parser *p);
static ASTNode *ParseSymbol(Parser *p);
static ASTNode *ParseString(Parser *p);
static ASTNode *ParseLiteral(Parser *p);

static ASTNode *ParseParams(Parser *p);

static ParseRule rules[] = {
  [eofToken]      = {0,             0,            precNone},
  [newlineToken]  = {0,             0,            precNone},
  [spaceToken]    = {0,             0,            precNone},
  [bangeqToken]   = {0,             ParseNotOp,   precEqual},
  [stringToken]   = {ParseString,   0,            precNone},
  [hashToken]     = {ParseUnary,    0,            precNone},
  [byteToken]     = {ParseByte,     0,            precNone},
  [percentToken]  = {0,             ParseOp,      precProduct},
  [ampToken]      = {0,             ParseOp,      precProduct},
  [lparenToken]   = {ParseGroup,    ParseCall,    precCall},
  [rparenToken]   = {0,             0,            precNone},
  [starToken]     = {0,             ParseOp,      precProduct},
  [plusToken]     = {0,             ParseOp,      precSum},
  [commaToken]    = {0,             0,            precNone},
  [minusToken]    = {ParseUnary,    ParseOp,      precSum},
  [arrowToken]    = {0,             0,            precNone},
  [dotToken]      = {0,             ParseOp,      precQualify},
  [slashToken]    = {0,             ParseOp,      precProduct},
  [numToken]      = {ParseNum,      0,            precNone},
  [hexToken]      = {ParseHex,      0,            precNone},
  [colonToken]    = {ParseSymbol,   ParsePair,    precPair},
  [ltToken]       = {0,             ParseOp,      precCompare},
  [ltltToken]     = {0,             ParseOp,      precShift},
  [lteqToken]     = {0,             ParseNotOp,   precCompare},
  [ltgtToken]     = {0,             ParseOp,      precJoin},
  [eqToken]       = {0,             0,            precNone},
  [eqeqToken]     = {0,             ParseOp,      precEqual},
  [gtToken]       = {0,             ParseOp,      precCompare},
  [gteqToken]     = {0,             ParseNotOp,   precCompare},
  [gtgtToken]     = {0,             ParseNegOp,   precShift},
  [atToken]       = {ParseUnary,    0,            precNone},
  [idToken]       = {ParseID,       0,            precNone},
  [andToken]      = {0,             ParseOp,      precLogic},
  [asToken]       = {0,             0,            precNone},
  [defToken]      = {0,             0,            precNone},
  [doToken]       = {ParseDo,       0,            precNone},
  [elseToken]     = {0,             0,            precNone},
  [endToken]      = {0,             0,            precNone},
  [exportToken]   = {0,             0,            precNone},
  [falseToken]    = {ParseLiteral,  0,            precNone},
  [ifToken]       = {ParseIf,       0,            precNone},
  [importToken]   = {0,             0,            precNone},
  [letToken]      = {ParseLet,      0,            precNone},
  [moduleToken]   = {0,             0,            precNone},
  [nilToken]      = {ParseLiteral,  0,            precNone},
  [notToken]      = {ParseUnary,    0,            precNone},
  [orToken]       = {0,             ParseOp,      precLogic},
  [recordToken]   = {0,             0,            precNone},
  [trapToken]     = {ParseTrap,     0,            precNone},
  [trueToken]     = {ParseLiteral,  0,            precNone},
  [lbraceToken]   = {ParseTuple,    0,            precNone},
  [bslashToken]   = {ParseLambda,   0,            precNone},
  [rbraceToken]   = {0,             0,            precNone},
  [caretToken]    = {ParseUnary,    0,            precNone},
  [lbracketToken] = {ParseList,     ParseAccess,  precCall},
  [barToken]      = {0,             ParseOp,      precSum},
  [rbracketToken] = {0,             0,            precNone},
  [tildeToken]    = {ParseUnary,    0,            precNone},
  [errorToken]    = {0,             0,            precNone}
};

ASTNode *ParseModule(char *source)
{
  Parser p;
  ASTNode *node, *name, *imports, *exports, *body;
  InitParser(&p, source);

  VSpacing(&p);
  node = MakeNode(moduleNode, &p);

  name = ParseModuleName(&p);
  if (IsErrorNode(name)) return ParseFail(node, name);
  NodePush(node, name);

  imports = ParseImports(&p);
  if (IsErrorNode(imports)) return ParseFail(node, imports);
  NodePush(node, imports);

  VSpacing(&p);
  exports = ParseExports(&p);
  if (IsErrorNode(exports)) return ParseFail(node, exports);
  NodePush(node, exports);

  if (NodeCount(exports) > 0 && name->type != idNode) {
    return ParseFail(node, ParseError("Unnamed modules can't export", &p));
  }

  VSpacing(&p);
  body = ParseStmts(&p);
  if (IsErrorNode(body)) return ParseFail(node, body);
  if (name->type == idNode) {
    NodePush(body, CloneNode(exports));
  }

  body = TransformImports(imports, body, &p);
  NodePush(node, body);

  if (!AtEnd(&p)) return Expected("End of file", node, &p);
  node->end = p.token.pos;

  return node;
}

static ASTNode *ParseModuleName(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(moduleToken, p)) return NilNode(p);
  Spacing(p);
  node = ParseID(p);
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *ParseImports(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(importToken, p)) return EmptyNode(p);
  VSpacing(p);
  node = ParseImportList(p);
  if (IsErrorNode(node)) return node;
  node->end = p->token.pos;
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->start = start;
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *ParseImportList(Parser *p)
{
  ASTNode *node, *import;
  node = MakeNode(tupleNode, p);
  do {
    VSpacing(p);
    import = ParseImport(p);
    if (IsErrorNode(import)) return ParseFail(node, import);
    NodePush(node, import);
  } while (MatchToken(commaToken, p));
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseImport(Parser *p)
{
  ASTNode *node, *name, *alias, *fns;
  node = MakeNode(importNode, p);

  name = ParseID(p);
  if (IsErrorNode(name)) return ParseFail(node, name);
  NodePush(node, name);
  Spacing(p);
  alias = ParseAlias(name, p);
  if (IsErrorNode(alias)) return ParseFail(node, alias);
  NodePush(node, alias);
  Spacing(p);
  fns = ParseFnList(p);
  if (IsErrorNode(fns)) return ParseFail(node, fns);
  NodePush(node, fns);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseAlias(ASTNode *name, Parser *p)
{
  if (!MatchToken(asToken, p)) return CloneNode(name);
  Spacing(p);
  return ParseID(p);
}

static ASTNode *ParseFnList(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(lparenToken, p)) return EmptyNode(p);
  node = ParseIDList(p);
  if (IsErrorNode(node)) return node;
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  node->start = start;
  node->end =  p->token.pos;
  return node;
}

static u32 CountUnqualifiedImports(ASTNode *imports)
{
  u32 i, count = 0;
  for (i = 0; i < NodeCount(imports); i++) {
    ASTNode *import = NodeChild(imports, i);
    count += NodeCount(NodeChild(import, 2));
  }
  return count;
}

static ASTNode *TransformImports(ASTNode *imports, ASTNode *body, Parser *p)
{
  u32 i, j, num_assigns = CountUnqualifiedImports(imports);
  u32 assigned = 0;
  ASTNode *node;

  if (num_assigns == 0) return body;

  node = NewNode(letNode, imports->start, imports->end, 0);
  NodePush(node, MakeTerminal(constNode, IntVal(num_assigns), p));

  for (i = 0; i < NodeCount(imports); i++) {
    u32 import_index = NodeCount(imports) - i - 1;
    ASTNode *import = NodeChild(imports, import_index);
    ASTNode *alias = NodeChild(import, 1);
    ASTNode *ids = NodeChild(import, 2);

    for (j = 0; j < NodeCount(ids); j++) {
      u32 id_index = NodeCount(ids) - j - 1;
      u32 assign_index = num_assigns - assigned - 1;
      ASTNode *var = CloneNode(NodeChild(ids, id_index));
      ASTNode *assign = NewNode(assignNode, var->start, var->end, 0);
      ASTNode *ref = NewNode(refNode, var->start, var->end, 0);
      NodePush(ref, CloneNode(alias));
      NodePush(ref, CloneNode(var));
      NodePush(assign, MakeTerminal(constNode, IntVal(assign_index), p));
      NodePush(assign, var);
      NodePush(assign, ref);
      NodePush(assign, body);
      body = assign;
      assigned += 1;
    }
  }

  NodePush(node, body);

  return node;
}

static ASTNode *ParseExports(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(exportToken, p)) return EmptyNode(p);
  VSpacing(p);
  node = ParseIDList(p);
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->type = tupleNode;
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

ASTNode *TransformDef(ASTNode *list)
{
  /*
  ; from this:
  def
    id
    params
    test
    expr

  ; to this (single definition, no guards)
  def
    id
    lambda
      params
      expr

  ; or this (general case)
  def
    id
    lambda
      params
      if
        test1
        expr1
        if
          test2
          expr2
          panic
  */
  u32 i, last_index, start, end;
  ASTNode *def, *id, *params, *test, *expr, *lambda, *guard, *alt;

  /* build a chain of if/else blocks starting with last def */

  /* last def is the first def without a guard (or the actual last one) */
  for (i = 0; i < NodeCount(list); i++) {
    def = NodeChild(list, i);
    last_index = i;
    test = NodeChild(def, 2);
    if (IsTrueNode(test)) break;
  }

  id = NodeChild(def, 0);
  params = NodeChild(def, 1);
  expr = NodeChild(def, 3);
  end = def->end;

  /* if there's only one def, skip building the if/else chain */
  if (def == NodeChild(list, 0) && IsTrueNode(test)) {
    lambda = NewNode(lambdaNode, def->start, def->end, 0);
    NodePush(lambda, params);
    NodePush(lambda, expr);
    NodeChild(def, 1) = lambda;
    VecPop(def->data.children);
    VecPop(def->data.children);
    FreeNode(test);
    for (i = 1; i < NodeCount(list); i++) {
      FreeNode(NodeChild(list, i));
    }
    return def;
  }

  /* if the last def has a guard, the last alt should panic */
  if (IsTrueNode(test)) {
    alt = expr;
    FreeNode(test);
  } else {
    u32 value = IntVal(Symbol("No matching function"));
    ASTNode *panic = NewNode(panicNode, def->start, def->end, 0);
    ASTNode *msg = NewNode(strNode, def->start, def->end, value);
    NodePush(panic, msg);
    alt = NewNode(ifNode, def->start, def->end, 0);
    NodePush(alt, test);
    NodePush(alt, expr);
    NodePush(alt, panic);
  }
  /* keep the children around for later */
  FreeNodeShallow(def);

  for (i = 0; i < last_index; i++) {
    u32 index = last_index - i - 1;
    def = NodeChild(list, index);
    test = NodeChild(def, 2);
    expr = NodeChild(def, 3);
    start = def->start;
    guard = NewNode(ifNode, def->start, def->end, 0);
    NodePush(guard, test);
    NodePush(guard, expr);
    NodePush(guard, alt);
    VecPop(def->data.children);
    VecPop(def->data.children);
    FreeNode(def);
    alt = guard;
  }

  for (i = last_index + 1; i < NodeCount(list); i++) {
    FreeNode(NodeChild(list, i));
  }

  lambda = NewNode(lambdaNode, start, end, 0);
  NodePush(lambda, params);
  NodePush(lambda, alt);

  def = NewNode(defNode, start, end, 0);
  NodePush(def, id);
  NodePush(def, lambda);

  return def;
}

typedef struct {
  ASTNode **stmts; /* vec */
  ASTNode **defs; /* vec */
  HashMap map;
} StmtParser;

static void InitStmtParser(StmtParser *sp)
{
  sp->stmts = 0;
  sp->defs = 0;
  InitHashMap(&sp->map);
}

static void DestroyStmtParser(StmtParser *sp)
{
  u32 i;
  for (i = 0; i < VecCount(sp->defs); i++) {
    FreeNodeShallow(sp->defs[i]);
  }
  FreeVec(sp->stmts);
  FreeVec(sp->defs);
  DestroyHashMap(&sp->map);
}

static ASTNode *ParseStmtsFail(StmtParser *sp, ASTNode *result)
{
  u32 i;
  for (i = 0; i < VecCount(sp->stmts); i++) {
    FreeNode(sp->stmts[i]);
  }
  while (VecCount(sp->defs)) {
    FreeNode(VecPop(sp->defs));
  }
  DestroyStmtParser(sp);
  return result;
}

static bool CheckDefParams(StmtParser *sp, ASTNode *stmt)
{
  u32 i;
  u32 name = NodeValue(NodeChild(stmt, 0));
  ASTNode *list, *def, *def_params, *stmt_params;

  if (!HashMapContains(&sp->map, name)) return true;
  i = HashMapGet(&sp->map, name);
  list = sp->defs[HashMapGet(&sp->map, name)];
  def = NodeChild(list, 0);
  def_params = NodeChild(def, 1);
  stmt_params = NodeChild(stmt, 1);

  if (NodeCount(def_params) != NodeCount(stmt_params)) return false;
  for (i = 0; i < NodeCount(def_params); i++) {
    u32 param1 = NodeValue(NodeChild(def_params, i));
    u32 param2 = NodeValue(NodeChild(stmt_params, i));
    if (param1 != param2) {
      return false;
    }
  }
  return true;
}

static void AddDefStmt(StmtParser *sp, ASTNode *stmt)
{
  u32 name = NodeValue(NodeChild(stmt, 0));
  if (HashMapContains(&sp->map, name)) {
    ASTNode *list = sp->defs[HashMapGet(&sp->map, name)];
    NodePush(list, stmt);
  } else {
    ASTNode *list = NewNode(tupleNode, stmt->start, stmt->end, 0);
    NodePush(list, stmt);
    HashMapSet(&sp->map, name, VecCount(sp->defs));
    VecPush(sp->defs, list);
  }
}

static ASTNode *ParseStmts(Parser *p)
{
  ASTNode *node, *stmt;
  StmtParser sp;
  u32 i, start;

  start = p->token.pos;

  /* Keep track of def statements separately */
  InitStmtParser(&sp);

  do {
    VSpacing(p);
    if (AtEnd(p) || CheckToken(endToken, p)) break;

    if (CheckToken(defToken, p)) {
      stmt = ParseDef(p);
    } else if (CheckToken(recordToken, p)) {
      stmt = ParseRecord(p);
    } else {
      stmt = ParseExpr(p);
    }

    if (IsErrorNode(stmt)) return ParseStmtsFail(&sp, stmt);

    if (stmt->type == defNode) {
      if (!CheckDefParams(&sp, stmt)) {
        /* multiply-defined def failed */
        ASTNode *error = ParseError("Parameter list mismatch", p);
        return ParseStmtsFail(&sp, ParseFail(stmt, error));
      }
      AddDefStmt(&sp, stmt);
    } else {
      VecPush(sp.stmts, stmt);
    }
  } while (!AtEnd(p) && CheckToken(newlineToken, p));

  node = MakeNode(doNode, p);
  node->start = start;
  node->end = p->token.pos;

  for (i = 0; i < VecCount(sp.defs); i++) {
    ASTNode *def = TransformDef(sp.defs[i]);
    NodePush(node, def);
  }

  for (i = 0; i < VecCount(sp.stmts); i++) {
    NodePush(node, sp.stmts[i]);
  }

  DestroyStmtParser(&sp);

  return node;
}

static ASTNode *ParseDef(Parser *p)
{
  ASTNode *node, *result;
  node = MakeNode(defNode, p);
  MatchToken(defToken, p);
  Spacing(p);
  result = ParseID(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  if (!MatchToken(lparenToken, p)) return Expected("(", node, p);
  VSpacing(p);
  result = ParseParams(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  VSpacing(p);
  result = ParseDefGuard(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  return node;
}

static ASTNode *ParseParams(Parser *p)
{
  if (CheckToken(idToken, p)) return ParseIDList(p);
  return EmptyNode(p);
}

static ASTNode *ParseDefGuard(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(whenToken, p)) return MakeTerminal(constNode, IntVal(1), p);
  VSpacing(p);
  node = ParseExpr(p);
  Spacing(p);
  if (!MatchToken(commaToken, p)) return Expected(",", node, p);
  return node;
}

static ASTNode *ParseRecord(Parser *p)
{
  ASTNode *node, *id, *params, *body;
  u32 i;
  node = MakeNode(defNode, p);
  MatchToken(recordToken, p);
  Spacing(p);
  id = ParseID(p);
  if (IsErrorNode(id)) return ParseFail(node, id);
  NodePush(node, id);
  if (!MatchToken(lparenToken, p)) return Expected("(", node, p);
  VSpacing(p);
  params = ParseParams(p);
  if (IsErrorNode(params)) return ParseFail(node, params);
  NodePush(node, params);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  NodePush(node, MakeTerminal(constNode, IntVal(1), p));

  body = MakeNode(tupleNode, p);
  for (i = 0; i < NodeCount(params); i++) {
    ASTNode *param = NodeChild(params, i);
    ASTNode *item = MakeNode(pairNode, p);
    ASTNode *key = MakeTerminal(symNode, IntVal(NodeValue(param)), p);
    ASTNode *value = CloneNode(param);
    NodePush(item, value);
    NodePush(item, key);
    NodePush(body, item);
  }

  NodePush(node, body);
  return node;
}

static ASTNode *ParsePrec(Precedence prec, Parser *p)
{
  ASTNode *expr;
  ParseRule rule = rules[p->token.type];
  if (!rule.prefix) return ParseError("Unexpected token", p);
  expr = rule.prefix(p);
  if (IsErrorNode(expr)) return expr;
  Spacing(p);
  rule = rules[p->token.type];
  while (rule.prec >= prec) {
    expr = rule.infix(expr, p);
    if (IsErrorNode(expr)) return expr;
    Spacing(p);
    rule = rules[p->token.type];
  }
  return expr;
}

NodeType OpNodeType(TokenType type)
{
  switch (type) {
  case bangeqToken:   return eqNode;
  case percentToken:  return remNode;
  case ampToken:      return bitandNode;
  case starToken:     return mulNode;
  case plusToken:     return addNode;
  case minusToken:    return subNode;
  case slashToken:    return divNode;
  case ltToken:       return ltNode;
  case ltltToken:     return shiftNode;
  case lteqToken:     return gtNode;
  case ltgtToken:     return joinNode;
  case eqeqToken:     return eqNode;
  case gtToken:       return gtNode;
  case gteqToken:     return ltNode;
  case gtgtToken:     return shiftNode;
  case andToken:      return andNode;
  case orToken:       return orNode;
  case lbracketToken: return sliceNode;
  case barToken:      return bitorNode;
  case colonToken:    return pairNode;
  case dotToken:      return refNode;
  default:            assert(false);
  }
}

static ASTNode *ParseOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(OpNodeType(token.type), p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, arg);
  return node;
}

static ASTNode *ParseNegOp(ASTNode *expr, Parser *p)
{
  ASTNode *node = ParseOp(expr, p);
  ASTNode *arg, *neg;
  if (IsErrorNode(node)) return node;

  arg = VecPop(node->data.children);
  neg = MakeNode(negNode, p);
  NodePush(neg, arg);
  NodePush(node, neg);
  return node;
}

static ASTNode *ParseNotOp(ASTNode *expr, Parser *p)
{
  ASTNode *node = ParseOp(expr, p);
  ASTNode *not;
  if (IsErrorNode(node)) return node;
  not = MakeNode(notNode, p);
  NodePush(not, node);
  return not;
}

static ASTNode *ParsePair(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);
  node = MakeNode(OpNodeType(token.type), p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, arg);
  NodePush(node, expr);
  return node;
}

static ASTNode *ParseCall(ASTNode *expr, Parser *p)
{
  ASTNode *node, *args;
  u32 start = p->token.pos;
  if (!MatchToken(lparenToken, p)) return Expected("(", expr, p);
  node = MakeNode(callNode, p);
  node->start = start;
  NodePush(node, expr);
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) {
    args = ParseItems(p);
    if (IsErrorNode(args)) return ParseFail(node, args);
    NodePush(node, args);
    if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  } else {
    NodePush(node, EmptyNode(p));
  }
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseTrap(Parser *p)
{
  ASTNode *node, *id;
  MatchToken(trapToken, p);
  Spacing(p);
  id = ParseID(p);
  if (IsErrorNode(id)) return id;
  node = ParseCall(id, p);
  if (IsErrorNode(node)) return node;
  node->type = trapNode;
  return node;
}

static ASTNode *ParseAccess(ASTNode *expr, Parser *p)
{
  ASTNode *node, *arg;
  Adv(p);
  node = MakeNode(accessNode, p);
  node->start = expr->start;
  NodePush(node, expr);
  VSpacing(p);

  arg = ParseExpr(p);
  if (IsErrorNode(arg)) return ParseFail(node, arg);
  NodePush(node, arg);

  VSpacing(p);
  if (MatchToken(commaToken, p)) {
    node->type = sliceNode;
    VSpacing(p);
    arg = ParseExpr(p);
    if (IsErrorNode(arg)) return ParseFail(node, arg);
    NodePush(node, arg);
    VSpacing(p);
  }
  if (!MatchToken(rbracketToken, p)) return Expected("]", node, p);
  node->end = p->token.pos;
  return node;
}

static i32 UnaryOpNodeType(TokenType type)
{
  switch (type) {
  case minusToken:  return negNode;
  case hashToken:   return lenNode;
  case tildeToken:  return compNode;
  case atToken:     return headNode;
  case caretToken:  return tailNode;
  case notToken:    return notNode;
  default:          assert(false);
  }
}

static ASTNode *ParseUnary(Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  if (token.type == notToken) VSpacing(p);
  arg = ParsePrec(precCall, p);
  if (IsErrorNode(arg)) return arg;
  node = MakeNode(UnaryOpNodeType(token.type), p);
  node->start = token.pos;
  NodePush(node, arg);
  return node;
}

static ASTNode *ParseLambda(Parser *p)
{
  ASTNode *params, *body, *node;
  node = MakeNode(lambdaNode, p);
  if (!MatchToken(bslashToken, p)) return Expected("\\", node, p);
  Spacing(p);
  params = ParseParams(p);
  if (IsErrorNode(params)) return ParseFail(node, params);
  NodePush(node, params);
  if (!MatchToken(arrowToken, p)) return Expected("->", node, p);
  VSpacing(p);
  body = ParseExpr(p);
  if (IsErrorNode(body)) return ParseFail(node, body);
  NodePush(node, body);
  return node;
}

static ASTNode *ParseGroup(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  node = ParseExpr(p);
  if (IsErrorNode(node)) return node;
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  return node;
}

static ASTNode *ParseDo(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  if (!MatchToken(doToken, p)) return Expected("do", node, p);
  VSpacing(p);
  node = ParseStmts(p);
  if (IsErrorNode(node)) return node;
  node->start = start;
  if (!MatchToken(endToken, p)) return Expected("end", node, p);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseLet(Parser *p)
{
  ASTNode *node = 0, *assigns;
  i32 start = p->token.pos;
  u32 num_assigns = 0;
  if (!MatchToken(letToken, p)) return Expected("let", node, p);
  node = MakeNode(letNode, p);
  node->start = start;
  VSpacing(p);
  if (!CheckToken(idToken, p)) return Expected("assignment", node, p);
  assigns = ParseAssigns(&num_assigns, p);
  if (IsErrorNode(assigns)) return ParseFail(node, assigns);
  NodePush(node, MakeTerminal(constNode, IntVal(num_assigns), p));
  NodePush(node, assigns);
  return node;
}

static ASTNode *ParseAssigns(u32 *num_assigns, Parser *p)
{
  ASTNode *node, *result;

  if (MatchToken(inToken, p)) {
    VSpacing(p);
    return ParseExpr(p);
  }
  (*num_assigns)++;

  node = MakeNode(assignNode, p);
  NodePush(node, MakeTerminal(constNode, IntVal((*num_assigns) - 1), p));
  result = ParseID(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  Spacing(p);
  if (!MatchToken(eqToken, p)) return Expected("=", node, p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);

  if (MatchToken(exceptToken, p)) {
    ASTNode *alt = MakeNode(ifNode, p);
    NodePush(node, alt);
    VSpacing(p);
    result = ParseExpr(p);
    if (IsErrorNode(result)) return ParseFail(node, result);
    NodePush(alt, result);
    VSpacing(p);
    if (!MatchToken(commaToken, p)) return Expected(",", node, p);
    VSpacing(p);
    result = ParseExpr(p);
    if (IsErrorNode(result)) return ParseFail(node, result);
    NodePush(alt, result);
    VSpacing(p);
    result = ParseAssigns(num_assigns, p);
    if (IsErrorNode(result)) return ParseFail(node, result);
    NodePush(alt, result);
  } else {
    VSpacing(p);
    if (!CheckToken(idToken, p) && !CheckToken(inToken, p)) {
      return ParseFail(node, ParseError("Expected assignments or \"in\"", p));
    }
    result = ParseAssigns(num_assigns, p);
    if (IsErrorNode(result)) return ParseFail(node, result);
    NodePush(node, result);
  }
  return node;
}

static ASTNode *ParseIf(Parser *p)
{
  if (!MatchToken(ifToken, p)) return Expected("if", 0, p);
  VSpacing(p);
  return ParseClauses(p);
}

static ASTNode *ParseClauses(Parser *p)
{
  ASTNode *node, *result;
  if (MatchToken(elseToken, p)) {
    VSpacing(p);
    return ParseExpr(p);
  }
  node = MakeNode(ifNode, p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  if (!MatchToken(commaToken, p)) return Expected(",", node, p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  result = ParseClauses(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseList(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  if (!MatchToken(lbracketToken, p)) return Expected("[", node, p);
  VSpacing(p);
  if (MatchToken(rbracketToken, p)) {
    node = NilNode(p);
  } else {
    node = ParseListTail(p);
    VSpacing(p);
    if (!MatchToken(rbracketToken, p)) return Expected("]", node, p);
  }
  node->start = start;
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseListTail(Parser *p)
{
  ASTNode *node, *item, *tail;

  item = ParseExpr(p);
  if (IsErrorNode(item)) return item;

  if (MatchToken(commaToken, p)) {
    VSpacing(p);
    tail = ParseListTail(p);
    if (IsErrorNode(tail)) return ParseFail(item, tail);
  } else {
    tail = NilNode(p);
  }

  node = MakeNode(pairNode, p);
  NodePush(node, tail);
  NodePush(node, item);
  node->end = tail->end;
  return node;
}

static ASTNode *ParseTuple(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(lbraceToken, p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (MatchToken(rbraceToken, p)) {
    node = MakeNode(tupleNode, p);
    node->start = start;
    return node;
  }
  node = ParseItems(p);
  if (IsErrorNode(node)) return node;
  node->type = tupleNode;
  node->start = start;
  if (!MatchToken(rbraceToken, p)) return Expected("}", node, p);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseNum(Parser *p)
{
  ASTNode *node;
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  if (!CheckToken(numToken, p)) return ParseError("Expected number", p);
  for (i = 0; i < token.length; i++) {
    i32 d;
    if (lexeme[i] == '_') continue;
    d = lexeme[i] - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
  }
  node = MakeTerminal(constNode, IntVal(n), p);
  Adv(p);
  return node;
}

static ASTNode *ParseHex(Parser *p)
{
  ASTNode *node;
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  for (i = 2; i < token.length; i++) {
    i32 d;
    if (lexeme[i] == '_') continue;
    d = IsDigit(lexeme[i]) ? lexeme[i] - '0' : lexeme[i] - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
  }
  node = MakeTerminal(constNode, IntVal(n), p);
  Adv(p);
  return node;
}

static ASTNode *ParseByte(Parser *p)
{
  ASTNode *node;
  u8 byte;
  char *lexeme = p->text + p->token.pos;
  if (IsSpace(lexeme[1]) || !IsPrintable(lexeme[1])) {
    return ParseError("Expected character", p);
  }
  byte = lexeme[1];
  node = MakeTerminal(constNode, IntVal(byte), p);
  Adv(p);
  return node;
}

static ASTNode *ParseSymbol(Parser *p)
{
  ASTNode *node;
  i32 sym;
  i32 start = p->token.pos;
  Token token;
  if (!MatchToken(colonToken, p)) return ParseError("Expected \":\"", p);
  token = p->token;
  sym = SymbolFrom(p->text + token.pos, token.length);
  node = MakeTerminal(symNode, IntVal(sym), p);
  node->start = start;
  node->end = token.pos + token.length;
  Adv(p);
  return node;
}

static ASTNode *ParseString(Parser *p)
{
  ASTNode *node;
  i32 sym;
  u32 i, j, len;
  char *str;
  Token token = p->token;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  for (i = 0, len = 0; i < token.length - 2; i++) {
    char ch = p->text[token.pos + i + 1];
    if (ch == '\\') i++;
    len++;
  }
  str = malloc(len+1);
  str[len] = 0;
  for (i = 0, j = 0; j < len; j++) {
    char ch = p->text[token.pos + i + 1];
    if (ch == '\\') {
      i++;
      ch = p->text[token.pos + i + 1];
    }
    str[j] = ch;
    i++;
  }
  sym = Symbol(str);
  free(str);
  node = MakeTerminal(strNode, IntVal(sym), p);
  Adv(p);
  return node;
}

static ASTNode *ParseLiteral(Parser *p)
{
  ASTNode *node;
  if (MatchToken(nilToken, p)) {
    node = MakeTerminal(constNode, 0, p);
  } else if (MatchToken(trueToken, p)) {
    node = MakeTerminal(constNode, IntVal(1), p);
  } else if (MatchToken(falseToken, p)) {
    node = MakeTerminal(constNode, IntVal(0), p);
  } else {
    assert(false);
  }
  return node;
}

static ASTNode *ParseItems(Parser *p)
{
  ASTNode *node = MakeNode(tupleNode, p);
  ASTNode *item = ParseExpr(p);
  if (IsErrorNode(item)) return ParseFail(node, item);
  NodePush(node, item);
  VSpacing(p);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsErrorNode(item)) return ParseFail(node, item);
    NodePush(node, item);
    VSpacing(p);
  }
  return node;
}

static ASTNode *ParseIDList(Parser *p)
{
  ASTNode *node, *id;
  node = MakeNode(tupleNode, p);
  id = ParseID(p);
  if (IsErrorNode(id)) return ParseFail(node, id);
  NodePush(node, id);
  Spacing(p);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsErrorNode(id)) return ParseFail(node, id);
    NodePush(node, id);
    Spacing(p);
  }
  return node;
}

static ASTNode *ParseID(Parser *p)
{
  i32 sym;
  ASTNode *node;

  if (p->token.type != idToken && !IsKeyword(p->token.type)) {
    return ParseError("Expected identifier", p);
  }
  sym = SymbolFrom(p->text + p->token.pos, p->token.length);
  node = MakeTerminal(idNode, sym, p);
  Adv(p);
  return node;
}

static void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) VSpacing(p);
}
