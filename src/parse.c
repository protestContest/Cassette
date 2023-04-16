#include "parse.h"
#include "parse_table.h"
#include "parse_syms.h"
#include "lex.h"
#include "mem.h"
#include <stdlib.h>

// #define DEBUG_PARSE

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

static bool IsInfix(u32 sym)
{
  switch (sym) {
  case ParseSymLambda:
  case ParseSymLogic:
  case ParseSymEquals:
  case ParseSymPair:
  case ParseSymCompare:
  case ParseSymSum:
  case ParseSymProduct:
    return true;
  default:
    return false;
  }
}

static Val AbstractNode(Parser *p, u32 sym, Val children)
{
  Val node = children;

  switch (sym) {
  case ParseSymProgram:
    node = MakePair(p->mem, SymbolFor("do"), Head(p->mem, children));
    break;

  case ParseSymParams:
    if (IsNil(Head(p->mem, children))) {
      node = Tail(p->mem, children);
    } else {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, children)));
    }
    break;

  case ParseSymDef_block:
    node = MakeList(p->mem, 3, ListAt(p->mem, children, 0),
                               ListAt(p->mem, children, 2),
                               ListAt(p->mem, children, 5));
    break;

  case ParseSymCall:
    if (IsPair(Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, children));
    }
    break;

  case ParseSymMl_call:
    if (IsPair(Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, children)));
    } else {
      node = MakeList(p->mem, 2, ListAt(p->mem, children, 0),
                                 ListAt(p->mem, children, 2));
    }
    break;

  case ParseSymStmts: {
    Val tail = MakePair(p->mem, ListAt(p->mem, children, 1), nil);
    node = ListAppend(p->mem, Head(p->mem, children), tail);
    break;
  }

  case ParseSymGroup:
    node = ListAt(p->mem, children, 1);
    break;

  case ParseSymDo_block:
    if (ListLength(p->mem, ListAt(p->mem, children, 1)) == 1) {
      node = Head(p->mem, ListAt(p->mem, children, 1));
    } else {
      node = ListAppend(p->mem, MakePair(p->mem, SymbolFor("do"), nil),
                                ListAt(p->mem, children, 1));
    }
    break;

  case ParseSymItems:
  case ParseSymEntries:
    node = ListAppend(p->mem, Head(p->mem, children), MakePair(p->mem, ListAt(p->mem, children, 1), nil));
    break;

  case ParseSymList:
    node = MakePair(p->mem, MakeSymbol(p->mem, "list"), ListAt(p->mem, children, 1));
    break;

  case ParseSymTuple:
    node = MakePair(p->mem, MakeSymbol(p->mem, "tuple"), ListAt(p->mem, children, 1));
    break;

  case ParseSymEntry:
    node = MakePair(p->mem, ListAt(p->mem, children, 0), ListAt(p->mem, children, 3));
    break;

  case ParseSymDict:
    node = MakePair(p->mem, MakeSymbol(p->mem, "dict"), ListAt(p->mem, children, 1));
    break;

  case ParseSymIf_block: {
    Val condition = ListAt(p->mem, children, 2);
    Val consequent = ListAt(p->mem, children, 5);
    if (ListLength(p->mem, consequent) == 1) {
      consequent = Head(p->mem, consequent);
    } else {
      consequent = MakePair(p->mem, SymbolFor("do"), consequent);
    }
    Val alternative = nil;
    if (ListLength(p->mem, children) > 8) {
      alternative = ListAt(p->mem, children, 8);
      if (ListLength(p->mem, alternative) == 1) {
        alternative = Head(p->mem, alternative);
      } else {
        alternative = MakePair(p->mem, SymbolFor("do"), alternative);
      }
    }
    node = MakeList(p->mem, 4, SymbolFor("if"), condition, consequent, alternative);
    break;
  }

  case ParseSymClause:
    node = MakePair(p->mem, ListAt(p->mem, children, 0),
                            ListAt(p->mem, children, 2));
    break;

  case ParseSymClauses:
    node = ListAppend(p->mem, Head(p->mem, children),
                              Tail(p->mem, children));
    break;

  case ParseSymCond_block:
    node = MakePair(p->mem, SymbolFor("cond"), ListAt(p->mem, children, 2));
    break;


  case ParseSymAssign: {
    node = MakePair(p->mem, ListAt(p->mem, children, 0), ListAt(p->mem, children, 2));
    break;
  }

  case ParseSymAssigns:
    if (IsList(p->mem, Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, Tail(p->mem, children))));
    } else {
      node = MakeList(p->mem, 2, ListAt(p->mem, children, 0), ListAt(p->mem, children, 3));
    }
    break;

  case ParseSymLet_stmt:
    if (IsList(p->mem, ListAt(p->mem, children, 2))) {
      node = MakeList(p->mem, 2, SymbolFor("let"), ListAt(p->mem, children, 2));
    } else {
      node = MakeList(p->mem, 2, SymbolFor("let"), MakePair(p->mem, ListAt(p->mem, children, 2), nil));
    }
    break;

  case ParseSymLambda:
  case ParseSymLogic:
  case ParseSymEquals:
  case ParseSymPair:
  case ParseSymCompare:
  case ParseSymSum:
  case ParseSymProduct:
    node = MakeList(p->mem, 3, ListAt(p->mem, children, 1),
                               ListAt(p->mem, children, 0),
                               ListAt(p->mem, children, 3));
    break;
  }

#ifdef DEBUG_PARSE
  Print(symbol_names[sym]);
  Print(": ");
  PrintVal(p->mem, children);


  Print(" -> ");
  PrintVal(p->mem, node);
  Print("\n");
#endif

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

static void Indent(u32 level, u32 lines, bool is_last)
{
  if (level == 0) return;

  for (u32 i = 0; i < level - 1; i++) {
    if ((lines >> (level - i - 1)) & 1) {
      Print("│ ");
    } else {
      Print("  ");
    }
  }
  if (is_last) Print("└─");
  else Print("├─");
}

static void PrintASTNode(Val node, u32 indent, u32 lines, bool last, Mem *mem)
{
  if (IsNil(node)) {
    Indent(indent, lines, last);
    Print("nil\n");
  } else if (IsList(mem, node)) {
    Indent(indent, lines, last);
    Print("┐\n");
    while (!IsNil(node)) {
      bool is_last = IsNil(Tail(mem, node)) || !IsList(mem, Tail(mem, node));
      u32 next_lines = (lines << 1);
      if (!is_last) {
        next_lines += 1;
      }
      if (IsPair(node)) {
        PrintASTNode(Head(mem, node), indent + 1, next_lines, is_last, mem);
      } else {
        PrintLn(".");
        PrintASTNode(node, indent + 1, next_lines, is_last, mem);
      }
      node = Tail(mem, node);
    }
  } else if (IsPair(node)) {
    Indent(indent, lines, last);
    PrintVal(mem, Head(mem, node));
    Print(" | ");
    PrintVal(mem, Tail(mem, node));
    Print("\n");
  } else {
    Indent(indent, lines, last);
    PrintVal(mem, node);
    Print("\n");
  }
}

void PrintAST(Val ast, Mem *mem)
{
  if (IsNil(ast)) return;
  PrintASTNode(ast, 0, 0, false, mem);
}
