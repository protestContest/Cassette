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
    VecPush(p->nodes,
      MakeNode(token.value,
              IntVal(token.line),
              IntVal(token.col), p->mem));
  }
  VecPush(p->stack, state);
  return ParseNext(p, NextToken(&p->lex));
}

static void ReduceNodes(Parser *p, u32 sym, u32 num)
{
  Val children = nil;
  for (u32 i = 0; i < num; i++) {
    Val child = VecPop(p->nodes);

    if (!IsNil(child) && (!IsNode(child, p->mem) || !IsNil(NodeVal(child, p->mem)))) {
      children = MakePair(p->mem, child, children);
    }
  }

  Val node = ParseNode(sym, children, p->mem);
  VecPush(p->nodes, node);

#if DEBUG_PARSE
  Print(GrammarSymbolName(sym));
  Print(": ");
  PrintNodes(children, p->mem);
  Print(" -> ");
  PrintNode(node, p->mem);
  Print("\n");
  Print("Stack:\n");
  for (u32 i = 0; i < VecCount(p->nodes); i++) {
    Print("  ");
    PrintNode(p->nodes[i], p->mem);
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
    SyntaxError(p->lex.src, "Unexpected token", token);
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

  if (next_state >= 0) {
#if DEBUG_PARSE
    Print("s");
    PrintInt(next_state);
    Print("\n");
#endif
    return Shift(p, next_state, token);
  } else if (reduction >= 0) {
    u32 num = reduction_sizes[state];
#if DEBUG_PARSE
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
#if DEBUG_AST
  PrintAST(ast, mem);
#endif
  return ast;
}

char *GrammarSymbolName(u32 sym)
{
  return symbol_names[sym];
}

Val SyntaxError(Source src, char *message, Token token)
{
  Print(IOFGRed);
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
  Print(IOFGReset);
  return SymbolFor("error");
}
