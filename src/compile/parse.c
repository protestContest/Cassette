#include "parse.h"
#include "project.h"
#include "runtime/primitives.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define ParseOk(node)       ItemResult(node)
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
static Result ParseDef(Parser *p, u32 pos);
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

static Node *MakeTerminal(NodeType type, u32 position, Val value);
static Node *MakeNode(NodeType type, u32 position);
static void SetNodeType(Node *node, NodeType type);
static void FreeNode(Node *node);
static bool IsTerminal(Node *node);
static Result ParseFail(Result result, Node *node);

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Node *prefix, Parser *p);

typedef struct {
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[] = {
  /* TokenEOF */        {0,            0,                 PrecNone},
  /* TokenNewline */    {0,            0,                 PrecNone},
  /* TokenBangEq */     {0,            ParseLeftAssoc,    PrecEqual},
  /* TokenStr */        {ParseString,  0,                 PrecNone},
  /* TokenHash */       {ParseUnary,   0,                 PrecNone},
  /* TokenPercent */    {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenAmpersand */  {0,            ParseLeftAssoc,    PrecBitwise},
  /* TokenLParen */     {ParseGroup,   ParseCall,         PrecCall},
  /* TokenRParen */     {0,            0,                 PrecNone},
  /* TokenStar */       {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenPlus */       {0,            ParseLeftAssoc,    PrecSum},
  /* TokenComma */      {0,            0,                 PrecNone},
  /* TokenMinus */      {ParseUnary,   ParseLeftAssoc,    PrecSum},
  /* TokenArrow */      {0,            0,                 PrecNone},
  /* TokenDot */        {0,            ParseAccess,       PrecAccess},
  /* TokenDotDot */     {0,            ParseLeftAssoc,    PrecAccess},
  /* TokenSlash */      {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenNum */        {ParseNum,     0,                 PrecNone},
  /* TokenColon */      {ParseSymbol,  0,                 PrecNone},
  /* TokenLt */         {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenLtLt */       {0,            ParseLeftAssoc,    PrecShift},
  /* TokenLtEq */       {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenLtGt */       {0,            ParseLeftAssoc,    PrecPair},
  /* TokenEq */         {0,            0,                 PrecNone},
  /* TokenEqEq */       {0,            ParseLeftAssoc,    PrecEqual},
  /* TokenGt */         {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenGtEq */       {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenGtGt */       {0,            ParseLeftAssoc,    PrecShift},
  /* TokenLBracket */   {ParseList,    0,                 PrecNone},
  /* TokenBackslash */  {ParseLambda,  0,                 PrecNone},
  /* TokenRBracket */   {0,            0,                 PrecNone},
  /* TokenCaret */      {0,            ParseLeftAssoc,    PrecBitwise},
  /* TokenAnd */        {0,            ParseLogic,        PrecLogic},
  /* TokenAs */         {0,            0,                 PrecNone},
  /* TokenCond */       {ParseCond,    0,                 PrecNone},
  /* TokenDef */        {0,            0,                 PrecNone},
  /* TokenDo */         {ParseDo,      0,                 PrecNone},
  /* TokenElse */       {0,            0,                 PrecNone},
  /* TokenEnd */        {0,            0,                 PrecNone},
  /* TokenFalse */      {ParseLiteral, 0,                 PrecNone},
  /* TokenIf */         {ParseIf,      0,                 PrecNone},
  /* TokenImport */     {0,            0,                 PrecNone},
  /* TokenIn */         {0,            ParseLeftAssoc,    PrecMember},
  /* TokenLet */        {0,            0,                 PrecNone},
  /* TokenModule */     {0,            0,                 PrecNone},
  /* TokenNil */        {ParseLiteral, 0,                 PrecNone},
  /* TokenNot */        {ParseUnary,   0,                 PrecNone},
  /* TokenOr */         {0,            ParseLogic,        PrecLogic},
  /* TokenTrue */       {ParseLiteral, 0,                 PrecNone},
  /* TokenLBrace */     {ParseBraces,  0,                 PrecNone},
  /* TokenBar */        {0,            ParseRightAssoc,   PrecPair},
  /* TokenRBrace */     {0,            0,                 PrecNone},
  /* TokenTilde */      {ParseUnary,   0,                 PrecNone},
  /* TokenID */         {ParseID,      0,                 PrecNone}
};

Result ParseFile(char *filename, SymbolTable *symbols)
{
  Parser p;
  Result result;
  char *source = ReadFile(filename);
  if (source == 0) return ErrorResult("Could not read file", filename, 0);
  InitParser(&p, symbols);
  p.filename = filename;
  result = Parse(source, &p);
  Free(source);
  return result;
}

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
  if (!result.ok) return ParseFail(result, module);
  NodePush(module, ResultItem(result));

  SkipNewlines(&p->lex);
  result = ParseImports(p);
  if (!result.ok) return ParseFail(result, module);
  NodePush(module, ResultItem(result));

  result = ParseStmts(p);
  if (!result.ok) return ParseFail(result, module);
  if (!MatchToken(TokenEOF, &p->lex)) return ParseFail(ParseError("Unexpected token", p), module);
  NodePush(module, ResultItem(result));

  NodePush(module, MakeTerminal(StringNode, TokenPos(&p->lex), Sym(p->filename, p->symbols)));
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
  Result result;
  Node *node = MakeNode(ListNode, TokenPos(&p->lex));

  while (MatchToken(TokenImport, &p->lex)) {
    u32 pos = TokenPos(&p->lex);
    Node *import_node = MakeNode(ImportNode, pos);
    NodePush(node, import_node);

    result = ParseID(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(import_node, ResultItem(result));
    if (MatchToken(TokenAs, &p->lex)) {
      u32 pos = TokenPos(&p->lex);
      if (MatchToken(TokenStar, &p->lex)) {
        NodePush(import_node, MakeTerminal(NilNode, pos, Nil));
      } else {
        result = ParseID(p);
        if (!result.ok) return ParseFail(result, node);
        NodePush(import_node, ResultItem(result));
      }
    } else {
      NodePush(import_node, ResultItem(result));
    }

    SkipNewlines(&p->lex);
  }

  return ParseOk(node);
}

#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
static Result ParseStmts(Parser *p)
{
  Result result;
  Node *node = MakeNode(DoNode, TokenPos(&p->lex));
  Node *assigns = MakeNode(ListNode, TokenPos(&p->lex));
  Node *stmts = MakeNode(ListNode, TokenPos(&p->lex));
  NodePush(node, assigns);
  NodePush(node, stmts);

  SkipNewlines(&p->lex);
  while (!EndOfBlock(LexPeek(&p->lex))) {
    u32 pos = TokenPos(&p->lex);

    if (MatchToken(TokenLet, &p->lex)) {
      SkipNewlines(&p->lex);
      while (!MatchToken(TokenNewline, &p->lex)) {
        Node *var;
        if (MatchToken(TokenEOF, &p->lex)) break;

        result = ParseAssign(p);
        if (!result.ok) return ParseFail(result, node);
        NodePush(stmts, ResultItem(result));

        var = NodeChild(ResultItem(result), 0);
        NodePush(assigns, var);

        if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
      }
    } else if (MatchToken(TokenDef, &p->lex)) {
      Node *var;
      result = ParseDef(p, pos);
      if (!result.ok) return ParseFail(result, node);
      NodePush(stmts, ResultItem(result));

      var = NodeChild(ResultItem(result), 0);
      NodePush(assigns, var);
    } else {
      result = ParseExpr(p);
      if (!result.ok) return ParseFail(result, node);
      NodePush(stmts, ResultItem(result));
    }

    SkipNewlines(&p->lex);
  }

  if (NumNodeChildren(stmts) == 0) {
    NodePush(stmts, MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  }

  return ParseOk(node);
}

static Result ParseAssign(Parser *p)
{
  Result result;
  Node *node = MakeNode(LetNode, TokenPos(&p->lex));

  result = ParseID(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) return ParseFail(ParseError("Expected \"=\"", p), node);
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

static Result ParseDef(Parser *p, u32 pos)
{
  Result result;
  Node *node = MakeNode(DefNode, pos);
  Node *lambda, *params;

  result = ParseID(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  lambda = MakeNode(LambdaNode, pos);
  NodePush(node, lambda);
  params = MakeNode(ListNode, TokenPos(&p->lex));
  NodePush(lambda, params);

  if (MatchToken(TokenLParen, &p->lex)) {
    SkipNewlines(&p->lex);
    if (!MatchToken(TokenRParen, &p->lex)) {
      do {
        SkipNewlines(&p->lex);
        result = ParseID(p);
        if (!result.ok) return ParseFail(result, node);
        NodePush(params, ResultItem(result));
      } while (MatchToken(TokenComma, &p->lex));
      SkipNewlines(&p->lex);
      if (!MatchToken(TokenRParen, &p->lex)) {
        return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
      }
    }
    SkipNewlines(&p->lex);
  }

  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(lambda, ResultItem(result));

  return ParseOk(node);
}

static Result ParsePrec(Precedence prec, Parser *p)
{
  Result result;

  if (!ExprNext(&p->lex)) return ParseError("Expected expression", p);
  result = ExprNext(&p->lex)(p);

  while (result.ok && PrecNext(&p->lex) >= prec) {
    result = rules[p->lex.token.type].infix(ResultItem(result), p);
  }

  return result;
}

static Result ParseLeftAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));

  NodePush(node, op);
  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

static Result ParseRightAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));

  NodePush(node, op);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec, p);
  if (!result.ok) {
    FreeAST(prefix);
    return ParseFail(result, node);
  }
  NodePush(node, ResultItem(result));
  NodePush(node, prefix);

  return ParseOk(node);
}

static Result ParseCall(Node *prefix, Parser *p)
{
  Result result;
  Node *node = MakeNode(CallNode, TokenPos(&p->lex));

  NodePush(node, prefix);

  MatchToken(TokenLParen, &p->lex);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return ParseFail(result, node);
      NodePush(node, ResultItem(result));
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRParen, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
    }
  }
  return ParseOk(node);
}

static Result ParseAccess(Node *prefix, Parser *p)
{
  Result result;
  Node *node = MakeNode(CallNode, TokenPos(&p->lex));
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;

  NodePush(node, prefix);

  if (LexPeek(&p->lex) == TokenID) {
    result = ParseID(p);
    if (!result.ok) return ParseFail(result, node);
    SetNodeType(ResultItem(result), SymbolNode);
    NodePush(node, ResultItem(result));
  } else {
    result = ParsePrec(prec+1, p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));
  }

  return ParseOk(node);
}

static Result ParseLogic(Node *prefix, Parser *p)
{
  Result result;
  Token token = NextToken(&p->lex);
  NodeType type = token.type == TokenAnd ? AndNode : OrNode;
  Node *node = MakeNode(type, TokenPos(&p->lex));
  Precedence prec = rules[token.type].prec;

  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

static Result ParseLambda(Parser *p)
{
  Node *node = MakeNode(LambdaNode, TokenPos(&p->lex));
  Node *params;
  Result result;

  if (!MatchToken(TokenBackslash, &p->lex)) {
    return ParseFail(ParseError("Expected \"\\\"", p), node);
  }

  params = MakeNode(ListNode, TokenPos(&p->lex));
  NodePush(node, params);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenArrow, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!result.ok) return ParseFail(result, node);
      NodePush(params, ResultItem(result));
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \"->\"", p), node);
    }
  }

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

static Result ParseUnary(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Node *node = MakeNode(CallNode, pos);
  Node *op = MakeTerminal(IDNode, pos, OpSymbol(token.type));

  NodePush(node, op);

  SkipNewlines(&p->lex);
  result = ParsePrec(PrecUnary, p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

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
  if (!MatchToken(TokenRParen, &p->lex)) {
    return ParseFail(ParseError("Expected \")\"", p), ResultItem(result));
  }
  return result;
}

static Result ParseDo(Parser *p)
{
  Result result;
  Node *node, *assigns, *stmts;
  u32 pos = TokenPos(&p->lex);
  Assert(MatchToken(TokenDo, &p->lex));

  result = ParseStmts(p);
  if (!result.ok) return result;
  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  node = ResultItem(result);
  node->pos = pos;
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
  Result result;
  Node *node = MakeNode(IfNode, TokenPos(&p->lex));

  Assert(MatchToken(TokenIf, &p->lex));

  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  if (!MatchToken(TokenDo, &p->lex)) {
    return ParseFail(ParseError("Expected \"do\"", p), node);
  }

  result = ParseStmts(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));
  } else {
    NodePush(node, MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  }

  if (!MatchToken(TokenEnd, &p->lex)) {
    return ParseFail(ParseError("Expected \"end\"", p), node);
  }

  return ParseOk(node);
}

static Result ParseClauses(Parser *p)
{
  if (MatchToken(TokenEnd, &p->lex)) {
    return ParseOk(MakeTerminal(NilNode, TokenPos(&p->lex), Nil));
  } else {
    Result result;
    Node *node = MakeNode(IfNode, TokenPos(&p->lex));

    result = ParseExpr(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));

    SkipNewlines(&p->lex);
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \"->\"", p), node);
    }
    SkipNewlines(&p->lex);

    result = ParseExpr(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));

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
  Result result;
  Node *node = MakeNode(ListNode, TokenPos(&p->lex));

  Assert(MatchToken(TokenLBracket, &p->lex));
  SkipNewlines(&p->lex);

  if (!MatchToken(TokenRBracket, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return ParseFail(result, node);
      NodePush(node, ResultItem(result));
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
  saved = p->lex;

  MatchToken(TokenLBrace, &p->lex);
  if (MatchToken(TokenColon, &p->lex)) {
    if (MatchToken(TokenRBrace, &p->lex)) {
      return ParseOk(MakeNode(MapNode, pos));
    }
  }
  p->lex = saved;

  MatchToken(TokenLBrace, &p->lex);
  SkipNewlines(&p->lex);
  if (MatchToken(TokenID, &p->lex)) {
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
  Assert(MatchToken(TokenLBrace, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseID(p);
    if (!result.ok) return ParseFail(result, node);
    SetNodeType(ResultItem(result), SymbolNode);
    NodePush(node, ResultItem(result));

    if (!MatchToken(TokenColon, &p->lex)) {
      return ParseFail(ParseError("Expected \":\"", p), node);
    }

    result = ParseExpr(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return ParseOk(node);
}

static Result ParseTuple(Parser *p)
{
  Node *node = MakeNode(TupleNode, TokenPos(&p->lex));
  Assert(MatchToken(TokenLBrace, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseExpr(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));

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
  SetNodeType(ResultItem(result), SymbolNode);

  return ParseOk(ResultItem(result));
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

static Node *MakeTerminal(NodeType type, u32 position, Val value)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  node->expr.value = value;
  return node;
}

static Node *MakeNode(NodeType type, u32 position)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  InitVec(&node->expr.children, sizeof(Node*), 2);
  return node;
}

static bool IsTerminal(Node *node)
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

static void SetNodeType(Node *node, NodeType type)
{
  node->type = type;
}

static void FreeNode(Node *node)
{
  if (!IsTerminal(node)) {
    DestroyVec(&node->expr.children);
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

static Result ParseFail(Result result, Node *node)
{
  FreeAST(node);
  return result;
}

static Val OpSymbol(TokenType token_type)
{
  switch (token_type) {
  case TokenBangEq:     return SymBangEq;
  case TokenHash:       return SymHash;
  case TokenPercent:    return SymPercent;
  case TokenAmpersand:  return SymAmpersand;
  case TokenStar:       return SymStar;
  case TokenPlus:       return SymPlus;
  case TokenMinus:      return SymMinus;
  case TokenDot:        return SymDot;
  case TokenDotDot:     return SymDotDot;
  case TokenSlash:      return SymSlash;
  case TokenLt:         return SymLt;
  case TokenLtLt:       return SymLtLt;
  case TokenLtEq:       return SymLtEq;
  case TokenLtGt:       return SymLtGt;
  case TokenEqEq:       return SymEqEq;
  case TokenGt:         return SymGt;
  case TokenGtEq:       return SymGtEq;
  case TokenGtGt:       return SymGtGt;
  case TokenCaret:      return SymCaret;
  case TokenIn:         return SymIn;
  case TokenNot:        return SymNot;
  case TokenBar:        return SymBar;
  case TokenTilde:      return SymTilde;
  default:              return Nil;
  }
}
