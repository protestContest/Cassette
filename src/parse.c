#include "parse.h"
#include "parse_table.h"
#include "lex.h"
#include "mem.h"
#include "ast.h"
#include "print.h"

static Val Shift(Parser *p, i32 state, Token token);
static Val Reduce(Parser *p, i32 sym, u32 num, Token token);
static Val ParseNext(Parser *p, Token token);

static Val Shift(Parser *p, i32 state, Token token)
{
  if (token.type == ParseSymEOF) {
    VecPush(p->nodes, nil);
  } else {
    VecPush(p->nodes, token.value);
  }
  VecPush(p->stack, state);
  token = NextToken(&p->lex);
  PrintTokenContext(p->lex.src, token, 1);
  return ParseNext(p, token);
}

static void ReduceNodes(Parser *p, u32 sym, u32 num)
{
  Val children = nil;
  for (u32 i = 0; i < num; i++) {
    Val child = VecPop(p->nodes);
    children = MakePair(p->mem, child, children);
  }

  Val node = ParseNode(sym, children, p->mem);
  VecPush(p->nodes, node);

#if DEBUG_AST
  Print(ParseSymbolName(sym));
  Print(": ");
  PrintVal(p->mem, children);
  Print(" -> ");
  PrintVal(p->mem, node);
  Print("\n");
  Print("Stack:\n");
  for (u32 i = 0; i < VecCount(p->nodes); i++) {
    Print("  ");
    PrintVal(p->mem, p->nodes[i]);
    Print("\n");
  }
  Print("\n");
#endif
}

static Val Reduce(Parser *p, i32 sym, u32 num, Token token)
{
  Assert(VecCount(p->nodes) >= num);
  Assert(VecCount(p->stack) >= num);

  i32 next_state = GetParseGoto(VecPeek(p->stack, num), sym);

#if DEBUG_PARSE
  Print("Goto ");
  PrintInt(next_state);
  Print("\n");
#endif

  if (next_state < 0) {
    return SyntaxError(p->lex.src, "Unexpected token", token);
  }

  ReduceNodes(p, sym, num);
  RewindVec(p->stack, num);
  VecPush(p->stack, next_state);

  return ParseNext(p, token);
}

static Val ParseNext(Parser *p, Token token)
{
#if DEBUG_PARSE
  for (u32 i = 0; i < VecCount(p->stack); i++) {
    PrintInt(p->stack[i]);
    Print(", ");
  }
  Print("\n");
#endif

  i32 state = VecPeek(p->stack, 0);

#if DEBUG_PARSE
  if (token.type == ParseSymEOF) {
    Print("$\t");
  } else {
    Print("\"");
    PrintToken(token);
    Print("\"\t");
  }
  Print("State ");
  PrintInt(state);
  Print(": ");
#endif

  i32 action = GetParseAction(state, token.type);
#if DEBUG_PARSE
  if (action >= 0) {
    Print("s");
    PrintInt(action);
    Print("\n");
  } else {
    Print("r");
    PrintInt(-action);
    Print("\n");
  }
#endif

  if (action == 0) {
    u32 num = GetReductionNum(state);
    ReduceNodes(p, action, num);
    RewindVec(p->stack, num);
    return VecPop(p->nodes);
  } else if (IsParseError(action)) {
    return SyntaxError(p->lex.src, "Unexpected token", token);
  } else if (action > 0) {
    return Shift(p, action, token);
  } else {
    return Reduce(p, -action, GetReductionNum(state), token);
  }
}

static void InitParser(Parser *p, Source src, Mem *mem)
{

  InitLexer(&p->lex, src, mem);
  p->stack = NULL;
  p->nodes = NULL;
  p->mem = mem;
  VecPush(p->stack, 0);
}

Val Parse(Source src, Mem *mem)
{
  Parser p;
  InitParser(&p, src, mem);
  Val ast = ParseNext(&p, NextToken(&p.lex));
  return MakePair(mem, MakeSymbol(mem, "ok"), ast);
}

Val ParseFile(char *filename, Mem *mem)
{
  Source src = ReadSourceFile(filename);
  if (src.name == NULL) {
    Print(src.data);
    return SymbolFor("error");
  }

  return Parse(src, mem);
}

Val SyntaxError(Source src, char *message, Token token)
{
  PrintEscape(IOFGRed);
  Print("(Syntax Error) ");
  if (token.type == ParseSymEOF) {
    Print("Unexpected end of input");
  } else {
    Print(message);
    Print(" '");
    PrintToken(token);
    Print("'");
  }
  Print("\n  ");
  PrintTokenPosition(src, token);
  Print("\n");
  PrintTokenContext(src, token, 5);
  PrintEscape(IOFGReset);
  return SymbolFor("error");
}
