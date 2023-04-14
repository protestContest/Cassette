#include "parse.h"
#include "parse_table.h"
#include "lex.h"
#include "mem.h"
#include <univ/vec.h>
#include <univ/io.h>
#include <stdlib.h>

#define DEBUG_PARSE

typedef struct {
  Lexer lex;
  i32 *stack;
  Val *nodes;
  Mem *mem;
} Parser;

static Val MakeNode(u32 parse_symbol, Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, (char*)symbol_names[parse_symbol]), children);
}

static bool IsOperator(Mem *mem, Val node)
{
  Val sym = Head(mem, node);
  if (Eq(sym, SymbolFor("+"))) return true;
  if (Eq(sym, SymbolFor("-"))) return true;
  if (Eq(sym, SymbolFor("*"))) return true;
  if (Eq(sym, SymbolFor("/"))) return true;
  return false;
}

static Val AbstractNode(Parser *p, u32 sym, Val children)
{
  Val node = nil;

  if (sym == ParseSymProgram) {
    node = MakePair(p->mem, MakeSymbol(p->mem, "do"), Head(p->mem, children));
  } else if (sym == ParseSymCall) {
    if (IsPair(Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, children));
    } else {
      node = children;
    }
  } else if (sym == ParseSymExpr) {
    node = children;
    if (ListLength(p->mem, node) == 1 && IsOperator(p->mem, Head(p->mem, node))) {
      node = Head(p->mem, node);
    }
  } else if (sym == ParseSymIf_block) {
    PrintVal(p->mem, children);
    Val condition = ListAt(p->mem, children, 1);
    Val consequent = MakePair(p->mem, SymbolFor("do"), ListAt(p->mem, children, 3));
    Val alternative = MakePair(p->mem, SymbolFor("do"), ListAt(p->mem, children, 5));
    node = MakeList(p->mem, 4, SymbolFor("if"), condition, consequent, alternative);
  } else if (sym == ParseSymSum ||
             sym == ParseSymProduct ||
             sym == ParseSymLambda) {
    node = Tail(p->mem, children);
    SetTail(p->mem, children, Tail(p->mem, node));
    SetTail(p->mem, node, children);
  } else if (sym == ParseSymGroup) {
    node = ListAt(p->mem, children, 1);
  } else if (sym == ParseSymNL) {
    node = nil;
  } else if (sym == ParseSymStmt) {
    node = Head(p->mem, children);
  } else if (sym == ParseSymBlock) {
    node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, children));
  } else if (sym == ParseSymDo_block) {
    node = ListAt(p->mem, children, 1);
  } else {
    Print("Unknown symbol \"");
    Print(symbol_names[sym]);
    Print("\", ");
    PrintVal(p->mem, children);
    Print("\n");
    exit(1);
  }

  return node;
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

  if (num == 1) {
    p->stack[VecCount(p->stack) - 1] = next_state;
  } else {
    ReduceNodes(p, sym, num);
    RewindVec(p->stack, num);
    VecPush(p->stack, next_state);
  }

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
    Print("No action for ");
    Print((char*)symbol_names[token.type]);
    Print(" in state ");
    PrintInt(state);
    Print("\n");
    return nil;
  }
}

static void InitParser(Parser *p, char *src, Mem *mem)
{
  InitLexer(&p->lex, RyeToken, src, mem);
  p->stack = NULL;
  p->nodes = NULL;
  p->mem = mem;
  VecPush(p->stack, 0);
}

Val Parse(char *src, Mem *mem)
{
  Parser p;
  InitParser(&p, src, mem);
  return ParseNext(&p, NextToken(&p.lex));
}

static void PrintASTNode(Val node, u32 indent, Mem *mem)
{
  if (IsNil(node)) return;

  if (IsPair(node)) {
    if (IsInt(Head(mem, node))) {
      u32 sym = RawInt(Head(mem, node));
      Print((char*)symbol_names[sym]);
      Print("\n");
    } else {
      PrintASTNode(Head(mem, node), indent, mem);
    }
    Val children = Tail(mem, node);
    while (!IsNil(children)) {
      for (u32 i = 0; i < indent + 1; i++) Print("  ");
      PrintASTNode(Head(mem, children), indent + 1, mem);
      children = Tail(mem, children);
    }
  } else {
    PrintVal(mem, node);
    Print("\n");
  }
}

void PrintAST(Val ast, Mem *mem)
{
  if (IsNil(ast)) return;
  PrintASTNode(ast, 0, mem);
}
