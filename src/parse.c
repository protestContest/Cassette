#include "parse.h"
#include "parse_table.h"
#include "parse_syms.h"
#include "lex.h"
#include "mem.h"
#include "ast.h"
#include <stdlib.h>

// #define DEBUG_PARSE

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
    Print("Syntax error: Unexpected token ");
    Print((char*)symbol_names[token.type]);
    Print(" at ");
    PrintInt(p->lex.line);
    Print(":");
    PrintInt(p->lex.col);
    Print(" (");
    PrintInt(p->lex.pos);
    Print(")");
    Print("\n");
    return nil;
  }
}

static void InitParser(Parser *p, char *src, Mem *mem)
{
  InitLexer(&p->lex, NUM_LITERALS, (Literal*)literals, src, mem);
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

static void Indent(u32 level, u32 lines)
{
  if (level == 0) return;

  for (u32 i = 0; i < level - 1; i++) {
    if ((lines >> (level - i - 1)) & 1) {
      Print("│");
    } else {
      Print(" ");
    }
  }
  // if (is_last) Print("└");
  // if (is_last) Print("└─");
  // else Print("├");
}

static void PrintASTNode(Val node, u32 indent, u32 lines, Mem *mem)
{
  if (IsNil(node)) {
    Print("╴");
    Print("nil\n");
  } else if (IsList(mem, node)) {
    Print("┬");
    u32 next_lines = (lines << 1);
    PrintASTNode(Head(mem, node), indent + 1, next_lines, mem);
    node = Tail(mem, node);
    while (!IsNil(node)) {
      bool is_last = IsNil(Tail(mem, node)) || !IsList(mem, Tail(mem, node));
      next_lines = (lines << 1);
      if (!is_last) {
        next_lines += 1;
      }

      Indent(indent + 1, next_lines);
      if (is_last) {
        Print("└");
      } else {
        Print("├");
      }
      PrintASTNode(Head(mem, node), indent + 1, next_lines, mem);
      node = Tail(mem, node);
    }
  } else if (IsPair(node)) {
    Print("╴");
    PrintVal(mem, Head(mem, node));
    Print(" | ");
    PrintVal(mem, Tail(mem, node));
    Print("\n");
  } else {
    Print("╴");
    PrintVal(mem, node);
    Print("\n");
  }
}

void PrintAST(Val ast, Mem *mem)
{
  if (IsNil(ast)) return;
  PrintASTNode(ast, 0, 0, mem);
}

char *GrammarSymbolName(u32 sym)
{
  return symbol_names[sym];
}

