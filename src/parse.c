#include "parse.h"
#include "parse_table.h"
#include "parse_syms.h"
#include "lex.h"
#include "mem.h"
#include "ast.h"

// #define DEBUG_PARSE

static Val SyntaxError(Source src, Token token);

static Val MakeNode(u32 parse_symbol, Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, (char*)symbol_names[parse_symbol]), children);
}

static Val Shift(Parser *p, i32 state, Token token);
static Val Reduce(Parser *p, u32 sym, u32 num, Token token);
static Val ParseNext(Parser *p, Token token);

static Val Shift(Parser *p, i32 state, Token token)
{
  VecPush(p->nodes, token.value);
  VecPush(p->stack, state);
  return ParseNext(p, NextToken(&p->lex));
}

static void ReduceNodes(Parser *p, u32 sym, u32 num)
{
  Val children = nil;
  for (u32 i = 0; i < num; i++) {
    Val child = VecPop(p->nodes, nil);
    children = MakePair(p->mem, child, children);
  }

  Val node = AbstractNode(p, sym, children);
  VecPush(p->nodes, node);

#ifdef DEBUG_PARSE
  Print("Stack:\n");
  for (u32 i = 0; i < VecCount(p->nodes); i++) {
    Print("  ");
    PrintVal(p->mem, p->nodes[i]);
    Print("\n");
  }
  Print("\n");
#endif
}

static Val Reduce(Parser *p, u32 sym, u32 num, Token token)
{
  Assert(VecCount(p->nodes) >= num);
  Assert(VecCount(p->stack) >= num);

  if (sym == TOP_SYMBOL) {
    ReduceNodes(p, sym, num);
    RewindVec(p->stack, num);
    return VecPop(p->nodes, nil);
  }

  i32 next_state = actions[VecPeek(p->stack, num)][sym];
  if (next_state < 0) {
    Print("Did not expect ");
    Print((char*)symbol_names[sym]);
    Print(" from state ");
    PrintInt(VecPeek(p->stack, 0));
    Print("\n");
    return nil;
  }

  ReduceNodes(p, sym, num);
  RewindVec(p->stack, num);
  VecPush(p->stack, next_state);

  return ParseNext(p, token);
}

static Val ParseNext(Parser *p, Token token)
{
  i32 state = VecPeek(p->stack, 0);
  i32 next_state = actions[state][token.type];
  i32 reduction = reduction_syms[state];

#ifdef DEBUG_PARSE
  Print("\"");
  PrintToken(token);
  Print("\"\t");
  Print("State ");
  PrintInt(state);
  Print(": ");
#endif

  if (next_state >= 0) {
#ifdef DEBUG_PARSE
    Print("s");
    PrintInt(next_state);
    Print("\n");
#endif
    return Shift(p, next_state, token);
  } else if (reduction >= 0) {
    u32 num = reduction_sizes[state];
#ifdef DEBUG_PARSE
    Print("r");
    PrintInt(reduction);
    Print(" (");
    Print(symbol_names[reduction]);
    Print(")\n");
#endif
    return Reduce(p, (u32)reduction, num, token);
  } else {
    return SyntaxError(p->lex.src, token);
  }
}

static void InitParser(Parser *p, Source src, Mem *mem)
{
  InitLexer(&p->lex, NUM_LITERALS, (Literal*)literals, src, mem);
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
#ifdef DEBUG_PARSE
  PrintAST(ast, mem);
#endif
  return ast;
}

char *GrammarSymbolName(u32 sym)
{
  return symbol_names[sym];
}

static Val SyntaxError(Source src, Token token)
{
  Print(IOFGRed);
  Print("(Syntax Error) ");
  if (token.type == 0) {
    Print("Unexpected end of input");
  } else {
    Print("Unexpected token '");
    PrintToken(token);
    Print("'");
  }
  Print("\n  ");
  PrintTokenPosition(src, token);
  Print("\n");
  PrintTokenContext(src, token, 5);
  Print(IOFGReset);
  return SymbolFor("error");
}
