#include "parse.h"
#include "mem.h"
#include "node.h"
#include "lex.h"
#include "module.h"
#include "ops.h"
#include <univ/math.h>
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/vec.h>

typedef struct {
  char *filename;
  char *text;
  Token token;
} Parser;
#define Adv(p) \
  (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))

#define Lexeme(token, p)    SymbolFrom((p)->text + (token).pos, (token).length)

Result ParseError(char *msg, Parser *p)
{
  Error *error = NewError(0, p->filename, p->token.pos, p->token.length);
  error->message = StrCat(error->message, "Parse error: ");
  error->message = StrCat(error->message, msg);
  return Err(error);
}

static void InitParser(Parser *p, char *text, char *filename)
{
  p->filename = filename;
  p->text = text;
  p->token.pos = 0;
  p->token.length = 0;
  Adv(p);
}

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

#define TokenNode(type, token, value, file) \
  MakeTerminal(type, file, (token).pos, (token).pos + (token).length, value)

typedef Result (*ParsePrefix)(Parser *p);
typedef Result (*ParseInfix)(ASTNode *expr, Parser *p);
typedef enum {
  precNone, precExpr, precLogic, precEqual, precPair, precJoin,
  precCompare, precShift, precSum, precProduct, precCall, precQualify
} Precedence;
typedef struct {
  ParsePrefix prefix;
  ParseInfix infix;
  Precedence prec;
} ParseRule;

static Result ParseModname(Module *module, Parser *p);
static Result ParseImports(ModuleImport **imports, Parser *p);
static Result ParseExports(ModuleExport **exports, Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseStmt(ASTNode ***stmts, ASTNode ***defs, Parser *p);
static Result ParseDef(Parser *p);
static Result ParseAssign(Parser *p);

#define ParseExpr(p)  ParsePrec(precExpr, p)
static Result ParsePrec(Precedence prec, Parser *p);
static Result ParseOp(ASTNode *expr, Parser *p);
static Result ParsePair(ASTNode *expr, Parser *p);
static Result ParseCall(ASTNode *expr, Parser *p);
static Result ParseAccess(ASTNode *expr, Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseLambda(Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseClauses(Parser *p);
static Result ParseList(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseVar(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseByte(Parser *p);
static Result ParseHex(Parser *p);
static Result ParseSymbol(Parser *p);
static Result ParseString(Parser *p);
static Result ParseLiteral(Parser *p);

static Result ParseID(Parser *p);
static Result ParseParams(Parser *p);
static void Spacing(Parser *p);
static void VSpacing(Parser *p);

static ParseRule rules[] = {
  [eofToken]      = {0,             0,            precNone},
  [newlineToken]  = {0,             0,            precNone},
  [spaceToken]    = {0,             0,            precNone},
  [bangeqToken]   = {0,             ParseOp,      precEqual},
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
  [colonToken]    = {ParseSymbol,   0,            precNone},
  [ltToken]       = {0,             ParseOp,      precCompare},
  [ltltToken]     = {0,             ParseOp,      precShift},
  [lteqToken]     = {0,             ParseOp,      precCompare},
  [ltgtToken]     = {0,             ParseOp,      precJoin},
  [eqToken]       = {0,             0,            precNone},
  [eqeqToken]     = {0,             ParseOp,      precEqual},
  [gtToken]       = {0,             ParseOp,      precCompare},
  [gteqToken]     = {0,             ParseOp,      precCompare},
  [gtgtToken]     = {0,             ParseOp,      precShift},
  [idToken]       = {ParseVar,      0,            precNone},
  [andToken]      = {0,             ParseOp,      precLogic},
  [asToken]       = {0,             0,            precNone},
  [condToken]     = {ParseCond,     0,            precNone},
  [defToken]      = {0,             0,            precNone},
  [doToken]       = {ParseDo,       0,            precNone},
  [elseToken]     = {0,             0,            precNone},
  [endToken]      = {0,             0,            precNone},
  [exportToken]   = {0,             0,            precNone},
  [falseToken]    = {ParseLiteral,  0,            precNone},
  [ifToken]       = {ParseIf,       0,            precNone},
  [importToken]   = {0,             0,            precNone},
  [letToken]      = {0,             0,            precNone},
  [moduleToken]   = {0,             0,            precNone},
  [nilToken]      = {ParseLiteral,  0,            precNone},
  [notToken]      = {ParseUnary,    0,            precNone},
  [orToken]       = {0,             ParseOp,      precLogic},
  [trueToken]     = {ParseLiteral,  0,            precNone},
  [lbraceToken]   = {ParseTuple,    0,            precNone},
  [bslashToken]   = {ParseLambda,   0,            precNone},
  [rbraceToken]   = {0,             0,            precNone},
  [caretToken]    = {0,             ParseOp,      precSum},
  [lbracketToken] = {ParseList,     ParseAccess,  precCall},
  [barToken]      = {0,             ParsePair,    precPair},
  [rbracketToken] = {0,             0,            precNone},
  [tildeToken]    = {ParseUnary,    0,            precNone},
  [errorToken]    = {0,             0,            precNone}
};

Result ParseModuleBody(Module *module)
{
  Parser p;
  Result result;

  InitParser(&p, module->source, module->filename);
  VSpacing(&p);

  result = ParseModname(module, &p);
  if (IsError(result)) return result;
  result = ParseImports(0, &p);
  if (IsError(result)) return result;
  result = ParseExports(0, &p);
  if (IsError(result)) return result;
  result = ParseStmts(&p);
  if (IsError(result)) return result;
  if (!AtEnd(&p)) return ParseError("Expected end of file", &p);
  module->ast = result.data.p;

  return Ok(module);
}

Result ParseModuleHeader(Module *module)
{
  Parser p;
  Result result;

  InitParser(&p, module->source, module->filename);
  VSpacing(&p);

  result = ParseModname(module, &p);
  if (IsError(result)) return result;
  result = ParseImports(&module->imports, &p);
  if (IsError(result)) return result;
  result = ParseExports(&module->exports, &p);
  if (IsError(result)) return result;

  return Ok(module);
}

static Result ParseModname(Module *module, Parser *p)
{
  if (!MatchToken(moduleToken, p)) {
    module->name = Symbol("*main*");
    return Ok(module);
  }
  Spacing(p);
  if (!CheckToken(idToken, p)) return ParseError("Expected module name", p);
  module->name = Lexeme(p->token, p);
  Adv(p);
  Spacing(p);
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Expected newline", p);
  }
  VSpacing(p);
  return Ok(module);
}

static Result ParseExports(ModuleExport **exports, Parser *p)
{
  ModuleExport export;
  if (!MatchToken(exportToken, p)) return Ok(0);
  VSpacing(p);
  if (!CheckToken(idToken, p)) return ParseError("Expected exports", p);
  do {
    VSpacing(p);
    if (!CheckToken(idToken, p)) return ParseError("Expected export", p);
    export.name = Lexeme(p->token, p);
    export.pos = p->token.pos;
    Adv(p);
    Spacing(p);
    if (exports) VecPush(*exports, export);
  } while (MatchToken(commaToken, p));


  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Missing comma or newline", p);
  }
  VSpacing(p);
  return Ok(0);
}

static Result ParseImports(ModuleImport **imports, Parser *p)
{
  if (!MatchToken(importToken, p)) return Ok(0);
  VSpacing(p);
  do {
    ModuleImport import;
    VSpacing(p);
    if (!CheckToken(idToken, p)) return ParseError("Expected import", p);
    import.module = Lexeme(p->token, p);
    import.pos = p->token.pos;
    Adv(p);
    Spacing(p);
    if (MatchToken(asToken, p)) {
      Spacing(p);
      if (!CheckToken(idToken, p)) return ParseError("Expected alias", p);
      import.alias = Lexeme(p->token, p);
      Adv(p);
      Spacing(p);
    } else {
      import.alias = import.module;
    }
    if (imports) VecPush(*imports, import);
  } while (MatchToken(commaToken, p));


  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Missing comma or newline", p);
  }
  VSpacing(p);
  return Ok(0);
}

static Result ParseStmts(Parser *p)
{
  Result result;
  ASTNode **stmts = 0;
  ASTNode **defs = 0;
  ASTNode *node;
  i32 pos = p->token.pos;
  result = ParseStmt(&stmts, &defs, p);
  if (IsError(result)) return result;
  while (!AtEnd(p) && CheckToken(newlineToken, p)) {
    VSpacing(p);
    if (AtEnd(p) || CheckToken(endToken, p) || CheckToken(elseToken, p)) break;
    result = ParseStmt(&stmts, &defs, p);
    if (IsError(result)) return result;
  }

  if (!stmts || VecLast(stmts)->type == letNode || VecLast(stmts)->type == defNode) {
    VecPush(stmts, MakeTerminal(constNode, p->filename, p->token.pos, p->token.pos, 0));
  }
  VSpacing(p);
  node = MakeNode(doNode, p->filename, pos, p->token.pos);
  if (defs) {
    u32 i;
    for (i = 0; i < VecCount(stmts); i++) {
      VecPush(defs, stmts[i]);
    }
    node->data.children = defs;
    FreeVec(stmts);
  } else {
    node->data.children = stmts;
  }
  return Ok(node);
}

static Result ParseStmt(ASTNode ***stmts, ASTNode ***defs, Parser *p)
{
  Result result;
  if (CheckToken(defToken, p)) {
    result = ParseDef(p);
    if (IsError(result)) return result;
    VecPush(*defs, result.data.p);
  } else if (MatchToken(letToken, p)) {
    Spacing(p);
    result = ParseAssign(p);
    if (IsError(result)) return result;
    VecPush(*stmts, result.data.p);
    while (MatchToken(commaToken, p)) {
      VSpacing(p);
      result = ParseAssign(p);
      if (IsError(result)) return result;
      VecPush(*stmts, result.data.p);
    }
  } else {
    result = ParseExpr(p);
    if (IsError(result)) return result;
    VecPush(*stmts, result.data.p);
  }
  return Ok(0);
}

static Result ParseDef(Parser *p)
{
  ASTNode *id, *lambda, *params, *body, *node;
  Result result;
  i32 pos = p->token.pos;
  if (!MatchToken(defToken, p)) return ParseError("Expected \"def\"", p);
  Spacing(p);
  result = ParseID(p);
  if (IsError(result)) return result;
  id = result.data.p;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  if (MatchToken(rparenToken, p)) {
    params = MakeNode(paramsNode, p->filename, p->token.pos, p->token.pos);
  } else {
    result = ParseParams(p);
    if (IsError(result)) return result;
    params = result.data.p;
    if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  }
  VSpacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  body = result.data.p;

  lambda = MakeNode(lambdaNode, p->filename, pos, body->end);
  NodePush(params, lambda);
  NodePush(body, lambda);

  node = MakeNode(defNode, p->filename, pos, lambda->end);
  NodePush(id, node);
  NodePush(lambda, node);
  return Ok(node);
}

static Result ParseAssign(Parser *p)
{
  ASTNode *id, *value, *node;
  Result result;
  result = ParseID(p);
  if (IsError(result)) return result;
  id = result.data.p;
  Spacing(p);
  if (!MatchToken(eqToken, p)) return ParseError("Expected \"=\"", p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  value = result.data.p;
  node = MakeNode(letNode, p->filename, id->start, value->end);
  NodePush(node, id);
  NodePush(node, value);
  return Ok(node);
}

static Result ParsePrec(Precedence prec, Parser *p)
{
  ASTNode *expr;
  Result result;
  ParseRule rule = rules[p->token.type];
  if (!rule.prefix) return ParseError("Unexpected token", p);
  result = rule.prefix(p);
  if (IsError(result)) return result;
  expr = result.data.p;
  rule = rules[p->token.type];
  while (rule.prec >= prec) {
    result = rule.infix(expr, p);
    if (IsError(result)) return result;
    expr = result.data.p;
    rule = rules[p->token.type];
  }
  return Ok(expr);
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
  case andToken:      return andNode;
  case orToken:       return orNode;
  case lbracketToken: return sliceNode;
  case caretToken:    return bitorNode;
  case barToken:      return pairNode;
  case dotToken:      return refNode;
  default:            assert(false);
  }
}

static Result ParseOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  i32 start, end;
  Result result;
  Adv(p);
  VSpacing(p);
  result = ParsePrec(rules[token.type].prec + 1, p);
  if (IsError(result)) return result;
  arg = result.data.p;
  start = expr->start;
  end = arg->end;

  if (token.type == gtgtToken) {
    ASTNode *newarg = MakeNode(negNode, p->filename, start, end);
    NodePush(arg, newarg);
    arg = newarg;
  }

  node = MakeNode(OpNodeType(token.type), p->filename, start, end);
  NodePush(expr, node);
  NodePush(arg, node);

  if (token.type == bangeqToken ||
      token.type == lteqToken ||
      token.type == gteqToken) {
    ASTNode *newnode = MakeNode(notNode, p->filename, start, end);
    NodePush(node, newnode);
    node = newnode;
  }
  return Ok(node);
}

static Result ParsePair(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  i32 start, end;
  Result result;
  Adv(p);
  VSpacing(p);
  result = ParsePrec(rules[token.type].prec, p);
  if (IsError(result)) return result;
  arg = result.data.p;
  start = expr->start;
  end = arg->end;
  node = MakeNode(OpNodeType(token.type), p->filename, start, end);
  NodePush(expr, node);
  NodePush(arg, node);
  return Ok(node);
}

static Result ParseCall(ASTNode *expr, Parser *p)
{
  Result result;
  ASTNode *node;
  Adv(p);
  VSpacing(p);

  node = MakeNode(callNode, p->filename, expr->start, expr->end);
  NodePush(expr, node);
  if (MatchToken(rparenToken, p)) {
    node->end = p->token.pos;
    Spacing(p);
    return Ok(node);
  }

  do {
    VSpacing(p);
    result = ParseExpr(p);
    if (IsError(result)) return result;
    NodePush(result.data.p, node);
  } while (MatchToken(commaToken, p));

  if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  node->end = p->token.pos;
  Spacing(p);
  return Ok(node);
}

static Result ParseAccess(ASTNode *expr, Parser *p)
{
  ASTNode *node, *start_node, *end_node = 0;
  u32 end;
  Result result;
  Adv(p);
  VSpacing(p);

  result = ParseExpr(p);
  if (IsError(result)) return result;
  start_node = result.data.p;

  VSpacing(p);
  if (MatchToken(colonToken, p)) {
    VSpacing(p);
    if (CheckToken(rbracketToken, p)) {
      end_node = MakeNode(lenNode, p->filename, p->token.pos, p->token.pos);
      NodePush(expr, end_node);
    } else {
      result = ParseExpr(p);
      if (IsError(result)) return result;
      end_node = result.data.p;
      VSpacing(p);
    }
  }
  if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  end = p->token.pos;
  Spacing(p);

  if (end_node) {
    node = MakeNode(sliceNode, p->filename, expr->start, end);
    NodePush(expr, node);
    NodePush(start_node, node);
    NodePush(end_node, node);
  } else {
    node = MakeNode(accessNode, p->filename, expr->start, end);
    NodePush(expr, node);
    NodePush(start_node, node);
  }
  return Ok(node);
}

i32 UnaryOpNodeType(TokenType type)
{
  switch (type) {
  case minusToken: return negNode;
  case hashToken: return lenNode;
  case tildeToken: return compNode;
  case notToken: return notNode;
  default: assert(false);
  }
}

static Result ParseUnary(Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Result result;
  Adv(p);
  if (token.type == notToken) VSpacing(p);
  result = ParsePrec(precCall, p);
  if (IsError(result)) return result;
  arg = result.data.p;
  node = MakeNode(UnaryOpNodeType(token.type), p->filename, token.pos, arg->end);
  NodePush(arg, node);
  return Ok(node);
}

static Result ParseLambda(Parser *p)
{
  ASTNode *params, *body, *node;
  Result result;
  i32 pos = p->token.pos;
  if (!MatchToken(bslashToken, p)) return ParseError("Expected \"\\\"", p);
  Spacing(p);
  if (MatchToken(arrowToken, p)) {
    params = MakeNode(paramsNode, p->filename, p->token.pos, p->token.pos);
  } else {
    result = ParseParams(p);
    if (IsError(result)) return result;
    params = result.data.p;
    if (!MatchToken(arrowToken, p)) return ParseError("Expected \"->\"", p);
  }
  VSpacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  body = result.data.p;
  node = MakeNode(lambdaNode, p->filename, pos, body->end);
  NodePush(params, node);
  NodePush(body, node);
  return Ok(node);
}

static Result ParseGroup(Parser *p)
{
  Result result;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  Spacing(p);
  return result;
}

static Result ParseDo(Parser *p)
{
  Result result;
  ASTNode *node;
  i32 start = p->token.pos, end;
  if (!MatchToken(doToken, p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  result = ParseStmts(p);
  if (IsError(result)) return result;
  node = result.data.p;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  end = p->token.pos;
  Spacing(p);
  node->start = start;
  node->end = end;
  return Ok(node);
}

static Result ParseIf(Parser *p)
{
  Result result;
  ASTNode *node;
  node = MakeNode(ifNode, p->filename, p->token.pos, p->token.pos);
  if (!MatchToken(ifToken, p)) return ParseError("Expected \"if\"", p);
  Spacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  NodePush(result.data.p, node);
  if (!MatchToken(doToken, p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  result = ParseStmts(p);
  if (IsError(result)) return result;
  NodePush(result.data.p, node);
  if (MatchToken(elseToken, p)) {
    VSpacing(p);
    result = ParseStmts(p);
    if (IsError(result)) return result;
    NodePush(result.data.p, node);
  } else {
    ASTNode *alt = MakeTerminal(constNode, p->filename, p->token.pos, p->token.pos, 0);
    NodePush(alt, node);
  }
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  node->end = p->token.pos;
  Spacing(p);
  return Ok(node);
}

static Result ParseCond(Parser *p)
{
  ASTNode *expr;
  Result result;
  if (!MatchToken(condToken, p)) return ParseError("Expected \"cond\"", p);
  result = ParseClauses(p);
  if (IsError(result)) return result;
  expr = result.data.p;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return Ok(expr);
}

static Result ParseClauses(Parser *p)
{
  ASTNode *node;
  Result result;
  VSpacing(p);
  if (CheckToken(endToken, p)) return Ok(TokenNode(constNode, p->token, 0, p->filename));

  node = MakeNode(ifNode, p->filename, p->token.pos, p->token.pos);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  NodePush(result.data.p, node);
  if (!MatchToken(arrowToken, p)) return ParseError("Expected \"->\"", p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsError(result)) return result;
  NodePush(result.data.p, node);
  node->end = p->token.pos;
  result = ParseClauses(p);
  if (IsError(result)) return result;
  NodePush(result.data.p, node);
  return Ok(node);
}

static Result ParseList(Parser *p)
{
  Result result;
  ASTNode *node, *tail;
  u32 start = p->token.pos;
  if (!MatchToken(lbracketToken, p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (MatchToken(rbraceToken, p)) {\
    node = MakeTerminal(constNode, p->filename, start, p->token.pos, 0);
    Spacing(p);
    return Ok(node);
  }

  node = MakeTerminal(constNode, p->filename, start, p->token.pos, 0);
  tail = node;

  do {
    ASTNode *item;
    VSpacing(p);
    result = ParseExpr(p);
    if (IsError(result)) return result;
    item = result.data.p;
    tail->type = pairNode;
    tail->start = item->start;
    tail->end = p->token.pos;
    NodePush(item, tail);
    NodePush(MakeTerminal(constNode, p->filename, p->token.pos, p->token.pos, 0), tail);
    tail = tail->data.children[1];
  } while (MatchToken(commaToken, p));

  if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  Spacing(p);

  return Ok(node);
}

static Result ParseTuple(Parser *p)
{
  Result result;
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(lbraceToken, p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (MatchToken(rbraceToken, p)) {
    node = MakeNode(tupleNode, p->filename, start, p->token.pos);
    Spacing(p);
    return Ok(node);
  }

  node = MakeNode(tupleNode, p->filename, start, start);

  do {
    VSpacing(p);
    result = ParseExpr(p);
    if (IsError(result)) return result;
    NodePush(result.data.p, node);
  } while (MatchToken(commaToken, p));

  if (!MatchToken(rbraceToken, p)) return ParseError("Expected \"}\"", p);
  node->end = p->token.pos;
  Spacing(p);
  return Ok(node);
}

static Result ParseVar(Parser *p)
{
  return ParseID(p);
}


static Result ParseNum(Parser *p)
{
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  if (!CheckToken(numToken, p)) return ParseError("Expected number", p);
  for (i = 0; i < token.length; i++) {
    i32 d = lexeme[i] - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
  }
  Adv(p);
  Spacing(p);
  return Ok(TokenNode(constNode, token, IntVal(n), p->filename));
}

static Result ParseHex(Parser *p)
{
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  for (i = 2; i < token.length; i++) {
    i32 d = IsDigit(lexeme[i]) ? lexeme[i] - '0' : lexeme[i] - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
  }
  Adv(p);
  Spacing(p);
  return Ok(TokenNode(constNode, token, IntVal(n), p->filename));
}

static Result ParseByte(Parser *p)
{
  u8 byte;
  i32 pos = p->token.pos, endPos = p->token.pos + p->token.length;
  char *lexeme = p->text + p->token.pos;
  if (IsSpace(lexeme[1]) || !IsPrintable(lexeme[1])) return ParseError("Expected character", p);
  byte = *lexeme;
  Adv(p);
  Spacing(p);
  return Ok(MakeTerminal(constNode, p->filename, pos, endPos, IntVal(byte)));
}

static Result ParseSymbol(Parser *p)
{
  i32 sym;
  Token token;
  if (!MatchToken(colonToken, p)) return ParseError("Expected \":\"", p);
  if (!CheckToken(idToken, p))return ParseError("Expected identifier", p);
  token = p->token;
  sym = SymbolFrom(p->text + token.pos, token.length);
  Adv(p);
  Spacing(p);
  return Ok(TokenNode(constNode, token, SymVal(sym), p->filename));
}

static Result ParseString(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  sym = SymbolFrom(p->text + token.pos + 1, token.length - 2);
  Adv(p);
  Spacing(p);
  return Ok(TokenNode(strNode, token, sym, p->filename));
}

static Result ParseLiteral(Parser *p)
{
  Token token = p->token;
  if (MatchToken(nilToken, p)) {
    Spacing(p);
    return Ok(TokenNode(constNode, token, 0, p->filename));
  } else if (MatchToken(trueToken, p)) {
    Spacing(p);
    return Ok(TokenNode(constNode, token, IntVal(1), p->filename));
  } else if (MatchToken(falseToken, p)) {
    Spacing(p);
    return Ok(TokenNode(constNode, token, IntVal(0), p->filename));
  } else {
    return ParseError("Unexpected expression", p);
  }
}

static Result ParseID(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(idToken, p)) return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + token.pos, token.length);
  Adv(p);
  Spacing(p);
  return Ok(TokenNode(varNode, token, sym, p->filename));
}

static Result ParseParams(Parser *p)
{
  Result result;
  ASTNode *node;

  node = MakeNode(paramsNode, p->filename, p->token.pos, p->token.pos);
  do {
    ASTNode *item;
    VSpacing(p);
    result = ParseID(p);
    if (IsError(result)) return result;
    item = result.data.p;
    NodePush(item, node);
    node->end = item->end;
  } while (MatchToken(commaToken, p));

  return Ok(node);
}

static void Spacing(Parser *p)
{
  MatchToken(spaceToken, p);
}

static void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) {
    VSpacing(p);
  }
}
