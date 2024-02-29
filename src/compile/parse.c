#include "parse.h"
#include "ast.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"


typedef struct {
  char *filename;
  Lexer lex;
} Parser;

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define ParseOk(node)       ItemResult(node)
#define ParseError(msg, p)  ErrorResult(msg, (p)->filename, (p)->lex.token.lexeme - (p)->lex.source)
#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
#define ParseExpr(p)        ParsePrec(PrecExpr, p)

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

static Result ParseModule(Parser *p);
static Node *WithExport(Node *node, Parser *p);
static Result ParseImports(Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseAssign(Parser *p);
static Result ParseDef(Parser *p, u32 pos);
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

static NodeType OpNode(TokenType token_type);

static Result ParseFail(Result result, Node *node);

/* A wrapper for ParseModule that handles opening/reading/closing the file */
Result ParseFile(char *filename)
{
  Parser p;
  Result result;
  char *source = ReadFile(filename);
  if (source == 0) return ErrorResult("Could not read file", filename, 0);
  p.filename = filename;
  InitLexer(&p.lex, source, 0);
  SkipNewlines(&p.lex);
  result = ParseModule(&p);
  Free(source);
  return result;
}

/* Parses a single expression */
Result Parse(char *source)
{
  Parser p;
  InitLexer(&p.lex, source, 0);
  SkipNewlines(&p.lex);
  return ParseExpr(&p);
}

/* Returns a ModuleNode */
static Result ParseModule(Parser *p)
{
  Result result;
  Node *module = MakeNode(ModuleNode, 0);
  char *mod_name;
  char *main = "*main*";

  /* module name */
  if (MatchToken(TokenModule, &p->lex)) {
    result = ParseID(p);
    if (!result.ok) return ParseFail(result, module);
    NodePush(module, ResultItem(result));
    mod_name = NodeValue(ResultItem(result));
  } else {
    NodePush(module, MakeTerminal(IDNode, 0, main));
    mod_name = main;
  }

  /* imports */
  SkipNewlines(&p->lex);
  result = ParseImports(p);
  if (!result.ok) return ParseFail(result, module);
  NodePush(module, ResultItem(result));

  /* body */
  result = ParseStmts(p);
  if (!result.ok) return ParseFail(result, module);
  if (!MatchToken(TokenEOF, &p->lex)) {
    return ParseFail(ParseError("Unexpected token", p), module);
  }

  if (mod_name == main) {
    NodePush(module, ResultItem(result));
  } else {
    NodePush(module, WithExport(ResultItem(result), p));
  }

  /* filename */
  NodePush(module, MakeTerminal(StringNode, TokenPos(&p->lex), p->filename));

  return ParseOk(module);
}

/* Adds an ExportNode to the end of a DoNode's statements */
static Node *WithExport(Node *node, Parser *p)
{
  Node *assigns = NodeChild(node, 0);
  Node *stmts = NodeChild(node, 1);
  Node *export = MakeNode(ExportNode, TokenPos(&p->lex));
  u32 i;

  NodePush(stmts, export);
  for (i = 0; i < NumNodeChildren(assigns); i++) {
    NodePush(export, NodeChild(assigns, i));
  }

  return node;
}

/* Returns a ListNode of ImportNodes */
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
        NodePush(import_node, MakeTerminal(NilNode, pos, 0));
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

/* Returns a DoNode */
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
    NodePush(stmts, MakeTerminal(NilNode, TokenPos(&p->lex), 0));
  }

  return ParseOk(node);
}

/* Returns a LetNode */
static Result ParseAssign(Parser *p)
{
  Result result;
  Node *node = MakeNode(LetNode, TokenPos(&p->lex));

  result = ParseID(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) {
    return ParseFail(ParseError("Expected \"=\"", p), node);
  }
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

/* Returns a DefNode */
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

/* Parses an expression according to the precedence rules table */
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

static Result ParseUnary(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Node *node = MakeNode(OpNode(token.type), pos);

  SkipNewlines(&p->lex);
  result = ParsePrec(PrecUnary, p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

static Result ParseLeftAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(OpNode(token.type), pos);

  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  return ParseOk(node);
}

/* Returns a CallNode */
static Result ParseRightAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(OpNode(token.type), pos);

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

/* Returns a CallNode or a SetNode */
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

/* Returns a CallNode */
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

/* Returns an AndNode or an OrNode */
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

/* Returns a LambdaNode */
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

/* Returns a node parsed at the highest precedence */
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

/* Returns a DoNode */
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

/* Returns an IfNode */
static Result ParseIf(Parser *p)
{
  Result result;
  Node *node = MakeNode(IfNode, TokenPos(&p->lex));

  Assert(MatchToken(TokenIf, &p->lex));

  /* predicate */
  result = ParseExpr(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  if (!MatchToken(TokenDo, &p->lex)) {
    return ParseFail(ParseError("Expected \"do\"", p), node);
  }

  /* consequent */
  result = ParseStmts(p);
  if (!result.ok) return ParseFail(result, node);
  NodePush(node, ResultItem(result));

  /* alternative */
  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!result.ok) return ParseFail(result, node);
    NodePush(node, ResultItem(result));
  } else {
    NodePush(node, MakeTerminal(NilNode, TokenPos(&p->lex), 0));
  }

  if (!MatchToken(TokenEnd, &p->lex)) {
    return ParseFail(ParseError("Expected \"end\"", p), node);
  }

  return ParseOk(node);
}

/* Returns an IfNode, where the alternative is an IfNode for the remaining
clauses, or a NilNode if it's the last clause */
static Result ParseClauses(Parser *p)
{
  if (MatchToken(TokenEnd, &p->lex)) {
    return ParseOk(MakeTerminal(NilNode, TokenPos(&p->lex), 0));
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

/* Begins the call to ParseClauses */
static Result ParseCond(Parser *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

/* Returns a ListNode */
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
    if (!MatchToken(TokenRBracket, &p->lex)) {
      return ParseError("Expected \")\"", p);
    }
  }

  return ParseOk(node);
}

/* Looks ahead to see if a tuple or map follows, then calls one of those.
Returns an empty MapNode in the special case `{:}`. */
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

/* Returns a MapNode */
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

/* Returns a TupleNode */
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

/* Returns an IDNode */
static Result ParseID(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  char *name;
  if (token.type != TokenID) return ParseError("Expected identifier", p);

  name = Alloc(token.length + 1);
  Copy(token.lexeme, name, token.length);
  name[token.length] = 0;
  return ParseOk(MakeTerminal(IDNode, pos, name));
}

/* Returns a NumNode */
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
      d = IsDigit(token.lexeme[i])
          ? token.lexeme[i] - '0'
          : 10 + token.lexeme[i] - 'A';
      whole = whole * 16 + d;
    }

    return ParseOk(MakeIntNode(pos, sign*whole));
  }

  if (*token.lexeme == '$') {
    u8 byte = token.lexeme[1];
    return ParseOk(MakeIntNode(pos, byte));
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
    return ParseOk(MakeFloatNode(pos, sign*num));
  } else {
    return ParseOk(MakeIntNode(pos, sign*whole));
  }
}

/* Returns a StringNode */
static Result ParseString(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  char *name = Alloc(token.length - 1);
  Copy(token.lexeme + 1, name, token.length - 2);
  name[token.length-1] = 0;
  return ParseOk(MakeTerminal(StringNode, pos, name));
}

/* Returns a SymbolNode */
static Result ParseSymbol(Parser *p)
{
  Result result;
  Assert(MatchToken(TokenColon, &p->lex));
  result = ParseID(p);
  if (!result.ok) return result;
  SetNodeType(ResultItem(result), SymbolNode);

  return ParseOk(ResultItem(result));
}

/* Returns a SymbolNode for `true` and `false`, or a NilNode for `nil` */
static Result ParseLiteral(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  char *trueVal = "true";
  char *falseVal = "false";

  switch (token.type) {
  case TokenTrue:   return ParseOk(MakeTerminal(SymbolNode, pos, trueVal));
  case TokenFalse:  return ParseOk(MakeTerminal(SymbolNode, pos, falseVal));
  case TokenNil:    return ParseOk(MakeTerminal(NilNode, pos, 0));
  default:          return ParseError("Unknown literal", p);
  }
}

/* Helper to deallocate a node and return a result */
static Result ParseFail(Result result, Node *node)
{
  FreeAST(node);
  return result;
}

/* Helper to map operator tokens to symbols, to use in a CallNode */
static NodeType OpNode(TokenType token_type)
{
  switch (token_type) {
  case TokenBangEq:     return NotEqNode;
  case TokenHash:       return LenNode;
  case TokenPercent:    return RemNode;
  case TokenStar:       return MulNode;
  case TokenPlus:       return AddNode;
  case TokenMinus:      return SubNode;
  case TokenDot:        return AccessNode;
  case TokenSlash:      return DivNode;
  case TokenLt:         return LtNode;
  case TokenLtEq:       return LtEqNode;
  case TokenEqEq:       return EqNode;
  case TokenGt:         return GtNode;
  case TokenGtEq:       return GtEqNode;
  case TokenIn:         return InNode;
  case TokenNot:        return NotNode;
  case TokenBar:        return PairNode;
  default:              return NilNode;
  }
}
