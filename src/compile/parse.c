#include "parse.h"
#include "project.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"
#include "runtime/primitives.h"

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define ParseOk(val)        DataResult(val)
#define ParseError(msg, p)  ErrorResult(msg, (p)->filename, (p)->lex.token.lexeme - (p)->lex.source)

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLambda,
  PrecLogic,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecBitwise,
  PrecShift,
  PrecSum,
  PrecProduct,
  PrecUnary,
  PrecCall,
  PrecAccess
} Precedence;

static Result ParseModuleName(Parser *p);
static Result ParseImports(Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseAssign(Parser *p);
static Result ParseDef(Parser *p);
#define ParseExpr(p) ParsePrec(PrecExpr, p)
static Result ParsePrec(Precedence prec, Parser *p);
static Result ParseLeftAssoc(Node *prefix, Parser *p);
static Result ParseRightAssoc(Node *prefix, Parser *p);
static Result ParseCall(Node *prefix, Parser *p);
static Result ParseAccess(Node *prefix, Parser *p);
static Result ParseLogic(Node *prefix, Parser *p);
static Result ParseLambda(Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseClause(Parser *p);
static Result ParseList(Parser *p);
static Result ParseBraces(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseMap(Parser *p);
static Result ParseLiteral(Parser *p);
static Result ParseSymbol(Parser *p);
static Result ParseID(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseString(Parser *p);

static Val OpSymbol(TokenType token_type);

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Node *prefix, Parser *p);

typedef struct {
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[] = {
  [TokenEOF]            = {0,            0,                 PrecNone},
  [TokenID]             = {ParseID,      0,                 PrecNone},
  [TokenBangEq]         = {0,            ParseLeftAssoc,    PrecEqual},
  [TokenStr]            = {ParseString,  0,                 PrecNone},
  [TokenNewline]        = {0,            0,                 PrecNone},
  [TokenHash]           = {ParseUnary,   0,                 PrecNone},
  [TokenPercent]        = {0,            ParseLeftAssoc,    PrecProduct},
  [TokenAmpersand]      = {0,            ParseLeftAssoc,    PrecBitwise},
  [TokenLParen]         = {ParseGroup,   ParseCall,         PrecCall},
  [TokenRParen]         = {0,            0,                 PrecNone},
  [TokenStar]           = {0,            ParseLeftAssoc,    PrecProduct},
  [TokenPlus]           = {0,            ParseLeftAssoc,    PrecSum},
  [TokenComma]          = {0,            0,                 PrecNone},
  [TokenMinus]          = {ParseUnary,   ParseLeftAssoc,    PrecSum},
  [TokenArrow]          = {0,            0,                 PrecNone},
  [TokenDot]            = {0,            ParseAccess,       PrecAccess},
  [TokenSlash]          = {0,            ParseLeftAssoc,    PrecProduct},
  [TokenNum]            = {ParseNum,     0,                 PrecNone},
  [TokenColon]          = {ParseSymbol,  0,                 PrecNone},
  [TokenLt]             = {0,            ParseLeftAssoc,    PrecCompare},
  [TokenLtLt]           = {0,            ParseLeftAssoc,    PrecShift},
  [TokenLtEq]           = {0,            ParseLeftAssoc,    PrecCompare},
  [TokenLtGt]           = {0,            ParseLeftAssoc,    PrecPair},
  [TokenEq]             = {0,            0,                 PrecNone},
  [TokenEqEq]           = {0,            ParseLeftAssoc,    PrecEqual},
  [TokenGt]             = {0,            ParseLeftAssoc,    PrecCompare},
  [TokenGtEq]           = {0,            ParseLeftAssoc,    PrecCompare},
  [TokenGtGt]           = {0,            ParseLeftAssoc,    PrecShift},
  [TokenLBracket]       = {ParseList,    0,                 PrecNone},
  [TokenBackslash]      = {ParseLambda,  0,                 PrecNone},
  [TokenRBracket]       = {0,            0,                 PrecNone},
  [TokenCaret]          = {0,            ParseLeftAssoc,    PrecBitwise},
  [TokenAnd]            = {0,            ParseLogic,        PrecLogic},
  [TokenAs]             = {0,            0,                 PrecNone},
  [TokenCond]           = {ParseCond,    0,                 PrecNone},
  [TokenDef]            = {0,            0,                 PrecNone},
  [TokenDo]             = {ParseDo,      0,                 PrecNone},
  [TokenElse]           = {0,            0,                 PrecNone},
  [TokenEnd]            = {0,            0,                 PrecNone},
  [TokenFalse]          = {ParseLiteral, 0,                 PrecNone},
  [TokenIf]             = {ParseIf,      0,                 PrecNone},
  [TokenImport]         = {0,            0,                 PrecNone},
  [TokenIn]             = {0,            ParseLeftAssoc,    PrecMember},
  [TokenLet]            = {0,            0,                 PrecNone},
  [TokenModule]         = {0,            0,                 PrecNone},
  [TokenNil]            = {ParseLiteral, 0,                 PrecNone},
  [TokenNot]            = {ParseUnary,   0,                 PrecNone},
  [TokenOr]             = {0,            ParseLogic,        PrecLogic},
  [TokenTrue]           = {ParseLiteral, 0,                 PrecNone},
  [TokenLBrace]         = {ParseBraces,  0,                 PrecNone},
  [TokenBar]            = {0,            ParseRightAssoc,   PrecPair},
  [TokenRBrace]         = {0,            0,                 PrecNone},
  [TokenTilde]          = {ParseUnary,   0,                 PrecNone}
};

void InitParser(Parser *p, SymbolTable *symbols)
{
  p->symbols = symbols;
}

Result Parse(char *source, Parser *p)
{
  Result result;
  Node *module = MakeNode(ModuleNode, 0);

  InitLexer(&p->lex, source, 0);
  SkipNewlines(&p->lex);

  result = ParseModuleName(p);
  if (!result.ok) return result;
  NodePush(module, result.data);

  SkipNewlines(&p->lex);
  result = ParseImports(p);
  if (!result.ok) return result;
  NodePush(module, result.data);

  result = ParseStmts(p);
  if (!result.ok) return result;
  if (!MatchToken(TokenEOF, &p->lex)) return ParseError("Unexpected token", p);
  NodePush(module, result.data);

  NodePush(module, MakeTerminal(StringNode, 0, Sym(p->filename, p->symbols)));
  return ParseOk(module);
}

static Result ParseModuleName(Parser *p)
{
  if (!MatchToken(TokenModule, &p->lex)) {
    return ParseOk(MakeTerminal(IDNode, 0, Sym(p->filename, p->symbols)));
  } else {
    return ParseID(p);
  }
}

static Result ParseImports(Parser *p)
{
  Node *node = MakeNode(ListNode, TokenPos(&p->lex));

  while (MatchToken(TokenImport, &p->lex)) {
    Node *import_node = MakeNode(ImportNode, TokenPos(&p->lex));
    Result result = ParseID(p);
    if (!result.ok) return result;
    NodePush(import_node, result.data);
    if (MatchToken(TokenAs, &p->lex)) {
      u32 pos = TokenPos(&p->lex);
      if (MatchToken(TokenStar, &p->lex)) {
        NodePush(import_node, MakeTerminal(NilNode, pos, Nil));
      } else {
        result = ParseID(p);
        if (!result.ok) return result;
        NodePush(import_node, result.data);
      }
    } else {
      NodePush(import_node, result.data);
    }

    NodePush(node, import_node);
    SkipNewlines(&p->lex);
  }

  return ParseOk(node);
}

#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
static Result ParseStmts(Parser *p)
{
  Node *node = MakeNode(DoNode, TokenPos(&p->lex));
  Node *assigns = MakeNode(ListNode, TokenPos(&p->lex));
  Node *stmts = MakeNode(ListNode, TokenPos(&p->lex));

  SkipNewlines(&p->lex);
  while (!EndOfBlock(LexPeek(&p->lex))) {
    Result result;

    if (MatchToken(TokenLet, &p->lex)) {
      SkipNewlines(&p->lex);
      while (!MatchToken(TokenNewline, &p->lex)) {
        Node *var;
        if (MatchToken(TokenEOF, &p->lex)) break;

        result = ParseAssign(p);
        if (!result.ok) return result;
        NodePush(stmts, result.data);

        var = NodeChild(result.data, 0);
        NodePush(assigns, var);

        if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
      }
    } else if (MatchToken(TokenDef, &p->lex)) {
      Node *var;
      result = ParseDef(p);
      if (!result.ok) return result;
      NodePush(stmts, result.data);

      var = NodeChild(result.data, 0);
      NodePush(assigns, var);
    } else {
      result = ParseExpr(p);
      if (!result.ok) return result;
      NodePush(stmts, result.data);
    }

    SkipNewlines(&p->lex);
  }

  if (NumNodeChildren(stmts) == 0) {
    NodePush(stmts, MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  }

  NodePush(node, assigns);
  NodePush(node, stmts);
  return ParseOk(node);
}

static Result ParseAssign(Parser *p)
{
  Node *node = MakeNode(LetNode, TokenPos(&p->lex));
  Result result;

  result = ParseID(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) return ParseError("Expected \"=\"", p);
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  return ParseOk(node);
}

static Result ParseDef(Parser *p)
{
  Node *node = MakeNode(DefNode, TokenPos(&p->lex));
  Node *lambda, *params;
  Result result;

  result = ParseID(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  lambda = MakeNode(LambdaNode, TokenPos(&p->lex));

  params = MakeNode(ListNode, TokenPos(&p->lex));
  if (MatchToken(TokenLParen, &p->lex)) {
    SkipNewlines(&p->lex);
    if (!MatchToken(TokenRParen, &p->lex)) {
      do {
        SkipNewlines(&p->lex);
        result = ParseID(p);
        if (!result.ok) return result;
        NodePush(params, result.data);
      } while (MatchToken(TokenComma, &p->lex));
      SkipNewlines(&p->lex);
      if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \",\" or \")\"", p);
    }
    SkipNewlines(&p->lex);
  }
  NodePush(lambda, params);

  result = ParseExpr(p);
  if (!result.ok) return result;
  NodePush(lambda, result.data);

  NodePush(node, lambda);
  return ParseOk(node);
}

static Result ParsePrec(Precedence prec, Parser *p)
{
  Result result;

  if (!ExprNext(&p->lex)) return ParseError("Expected expression", p);
  result = ExprNext(&p->lex)(p);

  while (result.ok && PrecNext(&p->lex) >= prec) {
    result = rules[p->lex.token.type].infix(result.data, p);
  }

  return result;
}

static Result ParseLeftAssoc(Node *prefix, Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));
  Result result;

  NodePush(node, op);
  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  return ParseOk(node);
}

static Result ParseRightAssoc(Node *prefix, Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));
  Result result;

  NodePush(node, op);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec, p);
  if (!result.ok) return result;
  NodePush(node, result.data);
  NodePush(node, prefix);

  return ParseOk(node);
}

static Result ParseCall(Node *prefix, Parser *p)
{
  Node *node = MakeNode(CallNode, TokenPos(&p->lex));
  Result result;

  NodePush(node, prefix);

  MatchToken(TokenLParen, &p->lex);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return result;
      NodePush(node, result.data);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \",\" or \")\"", p);
  }
  return ParseOk(node);
}

static Result ParseAccess(Node *prefix, Parser *p)
{
  Node *node = MakeNode(CallNode, TokenPos(&p->lex));
  Result result;
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;

  NodePush(node, prefix);

  if (LexPeek(&p->lex) == TokenID) {
    result = ParseID(p);
    if (!result.ok) return result;
    SetNodeType(result.data, SymbolNode);
    NodePush(node, result.data);
  } else {
    result = ParsePrec(prec+1, p);
    if (!result.ok) return result;
    NodePush(node, result.data);
  }

  return ParseOk(node);
}

static Result ParseLogic(Node *prefix, Parser *p)
{
  Token token = NextToken(&p->lex);
  NodeType type = token.type == TokenAnd ? AndNode : OrNode;
  Node *node = MakeNode(type, TokenPos(&p->lex));
  Precedence prec = rules[token.type].prec;
  Result result;

  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  return ParseOk(node);
}

static Result ParseLambda(Parser *p)
{
  Node *node = MakeNode(LambdaNode, TokenPos(&p->lex));
  Node *params;
  Result result;

  if (!MatchToken(TokenBackslash, &p->lex)) return ParseError("Expected \"\\\"", p);

  SkipNewlines(&p->lex);
  params = MakeNode(ListNode, TokenPos(&p->lex));
  if (!MatchToken(TokenArrow, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!result.ok) return result;
      NodePush(params, result.data);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenArrow, &p->lex)) return ParseError("Expected \",\" or \"->\"", p);
  }
  NodePush(node, params);

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  return ParseOk(node);
}

static Result ParseUnary(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));
  Result result;

  NodePush(node, op);

  SkipNewlines(&p->lex);
  result = ParsePrec(PrecUnary, p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  return ParseOk(node);
}

static Result ParseGroup(Parser *p)
{
  Result result;
  Assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!result.ok) return result;
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \")\"", p);
  return result;
}

static Result ParseDo(Parser *p)
{
  Result result;
  Node *node, *assigns, *stmts;
  Assert(MatchToken(TokenDo, &p->lex));

  result = ParseStmts(p);
  if (!result.ok) return result;
  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  node = result.data;
  assigns = NodeChild(node, 0);
  stmts = NodeChild(node, 1);

  if (NumNodeChildren(assigns) == 0 && NumNodeChildren(stmts) == 1) {
    Node *stmt = NodeChild(stmts, 0);
    FreeNode(assigns);
    FreeNode(stmts);
    FreeNode(node);
    return ParseOk(stmt);
  }

  return result;
}

static Result ParseIf(Parser *p)
{
  Node *node = MakeNode(IfNode, TokenPos(&p->lex));
  Result result;

  Assert(MatchToken(TokenIf, &p->lex));

  result = ParseExpr(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  result = ParseStmts(p);
  if (!result.ok) return result;
  NodePush(node, result.data);

  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!result.ok) return result;
    NodePush(node, result.data);
  } else {
    NodePush(node, MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  }

  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  return ParseOk(node);
}

static Result ParseClauses(Parser *p)
{

  if (MatchToken(TokenEnd, &p->lex)) {
    return ParseOk(MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  } else {
    Node *node = MakeNode(IfNode, TokenPos(&p->lex));
    Result result;

    result = ParseExpr(p);
    if (!result.ok) return result;
    NodePush(node, result.data);

    SkipNewlines(&p->lex);
    if (!MatchToken(TokenArrow, &p->lex)) return ParseError("Expected \"->\"", p);
    SkipNewlines(&p->lex);

    result = ParseExpr(p);
    if (!result.ok) return result;
    NodePush(node, result.data);

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!result.ok) return result;
    NodePush(node, result.data);

    return ParseOk(node);
  }
}

static Result ParseCond(Parser *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Result ParseList(Parser *p)
{
  Node *node = MakeNode(ListNode, TokenPos(&p->lex));
  Result result;

  Assert(MatchToken(TokenLBracket, &p->lex));
  SkipNewlines(&p->lex);

  if (!MatchToken(TokenRBracket, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return result;
      NodePush(node, result.data);
      SkipNewlines(&p->lex);
    } while(MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRBracket, &p->lex)) return ParseError("Expected \")\"", p);
  }

  return ParseOk(node);
}

static Result ParseBraces(Parser *p)
{
  Lexer saved;
  u32 pos = TokenPos(&p->lex);
  Assert(MatchToken(TokenLBrace, &p->lex));
  saved = p->lex;

  if (MatchToken(TokenColon, &p->lex)) {
    SkipNewlines(&p->lex);
    if (MatchToken(TokenRBrace, &p->lex)) {
      return ParseOk(MakeNode(MapNode, pos));
    }
  }
  p->lex = saved;

  SkipNewlines(&p->lex);
  if (MatchToken(TokenID, &p->lex)) {
    SkipNewlines(&p->lex);
    if (MatchToken(TokenColon, &p->lex)) {
      p->lex = saved;
      return ParseMap(p);
    }
  }

  p->lex = saved;
  return ParseTuple(p);
}

static Result ParseMap(Parser *p)
{
  Node *node = MakeNode(MapNode, TokenPos(&p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseID(p);
    if (!result.ok) return result;
    SetNodeType(result.data, SymbolNode);
    NodePush(node, result.data);

    if (!MatchToken(TokenColon, &p->lex)) return ParseError("Expected \":\"", p);

    result = ParseExpr(p);
    if (!result.ok) return result;
    NodePush(node, result.data);

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return ParseOk(node);
}

static Result ParseTuple(Parser *p)
{
  Node *node = MakeNode(TupleNode, TokenPos(&p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseExpr(p);
    if (!result.ok) return result;
    NodePush(node, result.data);

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return ParseOk(node);
}

static Result ParseID(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return ParseError("Expected identifier", p);
  return ParseOk(MakeTerminal(IDNode, pos, MakeSymbol(token.lexeme, token.length, p->symbols)));
}

static Result ParseNum(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  u32 whole = 0, frac = 0, frac_size = 1, i;
  i32 sign = 1;

  if (*token.lexeme == '-') {
    sign = -1;
    token.lexeme++;
    token.length--;
  }

  while (*token.lexeme == '0' || *token.lexeme == '_') {
    token.lexeme++;
    token.length--;
  }

  if (*token.lexeme == 'x') {
    u32 d;
    for (i = 1; i < token.length; i++) {
      if (token.lexeme[i] == '_') continue;
      d = IsDigit(token.lexeme[i]) ? token.lexeme[i] - '0' : 10 + token.lexeme[i] - 'A';
      whole = whole * 16 + d;
    }
    if (whole > RawInt(MaxIntVal)) return ParseError("Number overflows", p);
    return ParseOk(MakeTerminal(NumNode, pos, IntVal(sign*whole)));
  }

  if (*token.lexeme == '$') {
    u8 byte = token.lexeme[1];
    return ParseOk(MakeTerminal(NumNode, pos, IntVal(byte)));
  }

  for (i = 0; i < token.length; i++) {
    u32 d;
    if (token.lexeme[i] == '_') continue;
    if (token.lexeme[i] == '.') {
      i++;
      break;
    }
    d = token.lexeme[i] - '0';

    if (whole > MaxUInt/10) return ParseError("Number overflows", p);
    whole *= 10;

    if (whole > MaxUInt - d) return ParseError("Number overflows", p);
    whole += d;
  }
  for (; i < token.length; i++) {
    if (token.lexeme[i] == '_') continue;
    frac_size *= 10;
    frac  = frac * 10 + token.lexeme[i] - '0';
  }

  if (frac != 0) {
    float num = (float)whole + (float)frac / (float)frac_size;
    return ParseOk(MakeTerminal(NumNode, pos, FloatVal(sign*num)));
  } else {
    if (whole > RawInt(MaxIntVal)) return ParseError("Number overflows", p);
    return ParseOk(MakeTerminal(NumNode, pos, IntVal(sign*whole)));
  }
}

static Result ParseString(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, p->symbols);
  return ParseOk(MakeTerminal(StringNode, pos, symbol));
}

static Result ParseSymbol(Parser *p)
{
  Result result;
  Assert(MatchToken(TokenColon, &p->lex));
  result = ParseID(p);
  if (!result.ok) return result;
  SetNodeType(result.data, SymbolNode);

  return ParseOk(result.data);
}

static Result ParseLiteral(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);

  switch (token.type) {
  case TokenTrue:   return ParseOk(MakeTerminal(SymbolNode, pos, True));
  case TokenFalse:  return ParseOk(MakeTerminal(SymbolNode, pos, False));
  case TokenNil:    return ParseOk(MakeTerminal(NilNode, pos, Nil));
  default:          return ParseError("Unknown literal", p);
  }
}

Node *MakeTerminal(NodeType type, u32 position, Val value)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  node->expr.value = value;
  return node;
}

Node *MakeNode(NodeType type, u32 position)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  InitVec((Vec*)&node->expr.children, sizeof(Node*), 2);
  return node;
}

bool IsTerminal(Node *node)
{
  switch (node->type) {
  case NilNode:
  case IDNode:
  case NumNode:
  case StringNode:
  case SymbolNode:
    return true;
  default:
    return false;
  }
}

void SetNodeType(Node *node, NodeType type)
{
  node->type = type;
}

void FreeNode(Node *node)
{
  if (!IsTerminal(node)) {
    DestroyVec((Vec*)&node->expr.children);
  }

  Free(node);
}

void FreeAST(Node *node)
{
  if (!IsTerminal(node)) {
    u32 i;
    for (i = 0; i < NumNodeChildren(node); i++) {
      FreeNode(NodeChild(node, i));
    }
  }

  FreeNode(node);
}

static Val OpSymbol(TokenType token_type)
{
  switch (token_type) {
  case TokenBangEq:     return 0x7FDD5C4E;
  case TokenHash:       return 0x7FDF82DB;
  case TokenPercent:    return 0x7FD3E679;
  case TokenAmpersand:  return 0x7FDB283C;
  case TokenStar:       return 0x7FD9A24B;
  case TokenPlus:       return 0x7FD26AB0;
  case TokenMinus:      return 0x7FD9FBF9;
  case TokenDot:        return 0x7FD21A5F;
  case TokenSlash:      return 0x7FDDA21C;
  case TokenLt:         return 0x7FDD1F00;
  case TokenLtLt:       return 0x7FD72101;
  case TokenLtEq:       return 0x7FDE01F2;
  case TokenLtGt:       return 0x7FD3C54B;
  case TokenEqEq:       return 0x7FDC5014;
  case TokenGt:         return 0x7FD9FB4A;
  case TokenGtEq:       return 0x7FD7C966;
  case TokenGtGt:       return 0x7FDA0DDF;
  case TokenCaret:      return 0x7FDC17FE;
  case TokenIn:         return 0x7FD98998;
  case TokenNot:        return 0x7FDBCA20;
  case TokenBar:        return 0x7FDA1ADB;
  case TokenTilde:      return 0x7FD373CF;
  default:              return Nil;
  }
}
