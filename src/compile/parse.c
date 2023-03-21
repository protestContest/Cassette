#include "parse.h"
#include "lex.h"
#include <univ/base.h>
#include <univ/vec.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_STATES 43

#define ParseSymbolStr(sym) parse_symbol_names[sym]
static char *parse_symbol_names[] = {"End", "Error", "ID", "Num", "Arrow", "LParen", "RParen", "Minus", "Star", "Accept", "Expr", "Lambda", "Call", "IDs", "Args", "Group", "Sum", "Product", "Negative"};

#define TokenSym(token) token_syms[token]
static ParseSymbol token_syms[] = {
  [TOKEN_EOF] = SymEnd,
  [TOKEN_NUMBER] = SymNum,
  [TOKEN_ID] = SymID,
  [TOKEN_LPAREN] = SymLParen,
  [TOKEN_RPAREN] = SymRParen,
  [TOKEN_STAR] = SymStar,
  [TOKEN_SLASH] = SymSlash,
  [TOKEN_MINUS] = SymMinus,
  [TOKEN_PLUS] = SymPlus,
};

#define Shift(n)  -n
#define Reduce(n) n

static u32 default_table[NUM_STATES] = {
  0,    3,    4,    0,    0,    1,    2,    5,   12,   14,
  0,    0,   23,   17,   16,    0,    0,   15,    0,    0,
  0,    0,    0,   22,   11,   14,    8,   17,   16,    0,
 15,   10,   13,   18,   19,    0,    0,    7,    0,    9,
 20,   21,    6
};

static i32 parse_table[][NUM_TOKENS] = {
  [0] = {
    [TOKEN_ID] =      Shift(1),
    [TOKEN_NUMBER] =  Shift(2),
    [TOKEN_LPAREN] =  Shift(3),
  },
  [3] = {
    [TOKEN_ID]      = Shift(8),
    [TOKEN_NUMBER]  = Shift(9),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = Shift(10),
    [TOKEN_MINUS]   = Shift(11),
  },
  [4] = {
    [TOKEN_EOF]     = Shift(21)
  },
  [8] = {
    [TOKEN_STAR]    = Reduce(3)
  },
  [9] = {
    [TOKEN_STAR]    = Reduce(4)
  },
  [10] = {
    [TOKEN_ARROW]   = Shift(22)
  },
  [11] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
  },
  [13] = {
    [TOKEN_STAR]    = Reduce(1)
  },
  [14] = {
    [TOKEN_STAR]    = Reduce(2)
  },
  [15] = {
    [TOKEN_ID]      = Shift(24),
    [TOKEN_NUMBER]  = Shift(25),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = Shift(26),
  },
  [16] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = Shift(31),
  },
  [17] = {
    [TOKEN_STAR]    = Reduce(5)
  },
  [18] = {
    [TOKEN_RPAREN]  = Shift(33)
  },
  [19] = {
    [TOKEN_RPAREN]  = Shift(34),
    [TOKEN_MINUS]   = Shift(35),
  },
  [20] = {
    [TOKEN_STAR]    = Shift(36)
  },
  [22] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
  },
  [26] = {
    [TOKEN_ARROW]   = Shift(38)
  },
  [29] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = Shift(39),
  },
  [35] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_MINUS]   = Shift(11),
  },
  [36] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_MINUS]   = Shift(11),
  },
  [38] = {
    [TOKEN_ID]      = Shift(1),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_LPAREN]  = Shift(3),
  }
};

static u32 goto_table[][NUM_PARSE_SYM] = {
  [0] = {
    [SymExpr]     = 4,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
  [3] = {
    [SymExpr]     = 12,
    [SymLambda]   = 13,
    [SymCall]     = 14,
    [SymIDs]      = 15,
    [SymArgs]     = 16,
    [SymGroup]    = 17,
    [SymSum]      = 18,
    [SymProduct]  = 19,
    [SymNegative] = 20,
  },
  [11] = {
    [SymExpr]     = 23,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
  [15] = {
    [SymLambda]   = 27,
    [SymCall]     = 28,
    [SymArgs]     = 29,
    [SymGroup]    = 30,
  },
  [16] = {
    [SymExpr]     = 32,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
  [22] = {
    [SymExpr]     = 37,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
  [29] = {
    [SymExpr]     = 32,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
  [35] = {
    [SymExpr]     = 12,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7,
    [SymProduct]  = 40,
    [SymNegative] = 20,
  },
  [36] = {
    [SymExpr]     = 12,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7,
    [SymNegative] = 41,
  },
  [38] = {
    [SymExpr]     = 42,
    [SymLambda]   = 5,
    [SymCall]     = 6,
    [SymGroup]    = 7
  },
};

typedef struct {
  u32 num;
  ParseSymbol result;
} ReduceRule;

static ReduceRule rules[] = {
  [0]   = { 2, SymAccept },
  [1]   = { 1, SymExpr },
  [2]   = { 1, SymExpr },
  [3]   = { 1, SymExpr },
  [4]   = { 1, SymExpr },
  [5]   = { 1, SymExpr },
  [6]   = { 5, SymLambda },
  [7]   = { 4, SymLambda },
  [8]   = { 3, SymCall },
  [9]   = { 4, SymCall },
  [10]  = { 3, SymCall },
  [11]  = { 2, SymIDs },
  [12]  = { 1, SymIDs },
  [13]  = { 2, SymArgs },
  [14]  = { 1, SymArgs },
  [15]  = { 1, SymArgs },
  [16]  = { 1, SymArgs },
  [17]  = { 1, SymArgs },
  [18]  = { 3, SymGroup },
  [19]  = { 3, SymGroup },
  [20]  = { 3, SymSum },
  [21]  = { 3, SymProduct },
  [22]  = { 2, SymNegative },
  [23]  = { 1, SymNegative },
};

static u32 MakeNode(Parser *p, ParseSymbol sym, Token token)
{
  ASTNode node = {sym, token, NULL};
  VecPush(p->nodes, node);
  return VecCount(p->nodes) - 1;
}

void ParseNext(Parser *p)
{
  u32 state = VecLast(p->stack);
  printf("State %d: ", state);
  PrintToken(p->next_token);

  if (state > ArrayCount(parse_table)) {
    printf("Undefined state: %d\n", state);
    exit(1);
  }

  i32 action = default_table[state];
  if (action == 0) action = parse_table[state][p->next_token.type];

  if (action < 0) {
    // shift
    VecPush(p->stack, -action);
    printf("  Shift %d\n", -action);
    u32 sym = MakeNode(p, TokenSym(p->next_token.type), p->next_token);
    VecPush(p->symbols, sym);
    p->next_token = NextToken(&p->lex);
    ParseNext(p);
  } else if (action > 0) {
    // reduce
    printf("  Reduce %d\n", action);
    ReduceRule rule = rules[action];
    u32 sym = MakeNode(p, rule.result, p->next_token);
    for (u32 i = 0; i < rule.num; i++) {
      VecPush(p->nodes[sym].children, VecPeek(p->symbols, i));
    }
    RewindVec(p->symbols, rule.num);
    VecPush(p->symbols, sym);

    RewindVec(p->stack, rule.num);
    VecPush(p->stack, goto_table[VecLast(p->stack)][rule.result]);
    ParseNext(p);
  } else {
    printf("  Undefined action for token ");
    PrintToken(p->next_token);
    printf("\n");
  }
}

AST Parse(char *src)
{
  Parser p;
  InitLexer(&p.lex, RyeToken, src);
  p.next_token = NextToken(&p.lex);
  p.stack = NULL;
  p.symbols = NULL;
  p.nodes = NULL;
  VecPush(p.stack, 0);

  ParseNext(&p);
  AST ast = { p.nodes, VecLast(p.symbols) };

  FreeVec(p.stack);
  FreeVec(p.symbols);
  return ast;
}

static void PrintASTNode(AST ast, u32 node_id, u32 indent)
{
  for (u32 i = 0; i < indent; i++) printf("  ");

  ParseSymbol sym = ast.nodes[node_id].sym;
  printf("%s", parse_symbol_names[sym]);
  if (sym == SymID || sym == SymNum) {
    printf(" %.*s", ast.nodes[node_id].token.length, ast.nodes[node_id].token.lexeme);
  }
  printf("\n");

  for (u32 i = 0; i < VecCount(ast.nodes[node_id].children); i++) {
    PrintASTNode(ast, ast.nodes[node_id].children[i], indent + 1);
  }
}

void PrintAST(AST ast)
{
  PrintASTNode(ast, ast.root, 0);
}
