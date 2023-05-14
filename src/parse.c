#include "parse.h"
#include "parse_table.h"
#include "parse_syms.h"
#include "lex.h"
#include "mem.h"
#include "ast.h"

static Val Shift(Parser *p, i32 state, Token token);
static Val Reduce(Parser *p, u32 sym, u32 num, Token token);
static Val ParseNext(Parser *p, Token token);

static Val Shift(Parser *p, i32 state, Token token)
{
  if (token.type == ParseSymEOF) {
    VecPush(p->nodes, nil);
  } else {
    VecPush(p->nodes, token.value);
  }
  VecPush(p->stack, state);
  return ParseNext(p, NextToken(&p->lex));
}

static void ReduceNodes(Parser *p, u32 sym, u32 num)
{
  Val children = nil;
  for (u32 i = 0; i < num; i++) {
    Val child = VecPop(p->nodes);
    if (!IsNil(child)) {
      children = MakePair(p->mem, child, children);
    }
  }

  Val node = ParseNode(sym, children, p->mem);
  VecPush(p->nodes, node);

#if DEBUG_PARSE
  Print(GrammarSymbolName(sym));
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

static Val Reduce(Parser *p, u32 sym, u32 num, Token token)
{
  Assert(VecCount(p->nodes) >= num);
  Assert(VecCount(p->stack) >= num);

  if (sym == TOP_SYMBOL) {
    ReduceNodes(p, sym, num);
    RewindVec(p->stack, num);
    return VecPop(p->nodes);
  }

  i32 next_state = actions[VecPeek(p->stack, num)][sym];
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
  i32 state = VecPeek(p->stack, 0);
  i32 next_state = actions[state][token.type];
  i32 reduction = reduction_syms[state];

#if DEBUG_PARSE2
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

  if (next_state >= 0) {
#if DEBUG_PARSE2
    Print("s");
    PrintInt(next_state);
    Print("\n");
#endif
    return Shift(p, next_state, token);
  } else if (reduction >= 0) {
    u32 num = reduction_sizes[state];
#if DEBUG_PARSE2
    Print("r");
    PrintInt(reduction);
    Print(" (");
    Print(symbol_names[reduction]);
    Print(") -> ");
    PrintInt(VecPeek(p->stack, num));
    Print("\n");
#endif
    return Reduce(p, (u32)reduction, num, token);
  } else {
    return SyntaxError(p->lex.src, "Unexpected token", token);
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
  return MakePair(mem, SymbolFor("ok"), ast);
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

char *GrammarSymbolName(u32 sym)
{
  return symbol_names[sym];
}

Val SyntaxError(Source src, char *message, Token token)
{
  PrintEscape(IOFGRed);
  Print("(Syntax Error) ");
  if (token.type == 0) {
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
