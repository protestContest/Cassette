#include "parse.h"
#include <univ/vec.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
  ParseError,
  ParseStop,
  ParseShift,
  ParseReduce
} ParseAction;

static char *ParseSymbolStr(ParseSymbol sym)
{
  switch (sym) {
  case Number:      return "Number";
  case Identifier:  return "Identifier";
  case Value:       return "Value";
  case Primary:     return "Primary";
  case Sum:         return "Sum";
  case Product:     return "Product";
  case Expr:        return "Expr";
  }
}

static ParseSymbol TokenToParseSymbol(TokenType type)
{
  switch (type) {
  case TOKEN_EOF:
  case TOKEN_NUMBER:  return Number;
  case TOKEN_ID:      return Identifier;
  case TOKEN_LPAREN:  return Primary;
  case TOKEN_RPAREN:  return Primary;
  case TOKEN_STAR:    return Product;
  case TOKEN_SLASH:   return Product;
  case TOKEN_MINUS:   return Sum;
  case TOKEN_PLUS:    return Sum;
  }
  exit(1);
}

#define Shift(state)  (((state) << 2) + 2)
#define Reduce(rule)  (((rule) << 2) + 3)

#define ActionFor(n)  ((n) & 0x3)
#define ActionVal(n)  ((n) >> 2)

static u32 actions[][NUM_TOKENS] = {
  [0] = { // $accept: . expr $end
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [1] = { // value: id .
    [TOKEN_EOF]     = Reduce(11),
    [TOKEN_NUMBER]  = Reduce(11),
    [TOKEN_ID]      = Reduce(11),
    [TOKEN_LPAREN]  = Reduce(11),
    [TOKEN_RPAREN]  = Reduce(11),
    [TOKEN_STAR]    = Reduce(11),
    [TOKEN_SLASH]   = Reduce(11),
    [TOKEN_MINUS]   = Reduce(11),
    [TOKEN_PLUS]    = Reduce(11),
  },
  [2] = { // value: num .
    [TOKEN_EOF]     = Reduce(12),
    [TOKEN_NUMBER]  = Reduce(12),
    [TOKEN_ID]      = Reduce(12),
    [TOKEN_LPAREN]  = Reduce(12),
    [TOKEN_RPAREN]  = Reduce(12),
    [TOKEN_STAR]    = Reduce(12),
    [TOKEN_SLASH]   = Reduce(12),
    [TOKEN_MINUS]   = Reduce(12),
    [TOKEN_PLUS]    = Reduce(12),
  },
  [3] = { // primary: '(' . expr ')'
    [TOKEN_EOF]     = Reduce(2),
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = Reduce(2),
    [TOKEN_STAR]    = Reduce(2),
    [TOKEN_SLASH]   = Reduce(2),
    [TOKEN_MINUS]   = Reduce(2),
    [TOKEN_PLUS]    = Reduce(2),
  },
  [4] = { // $accept: expr . $end
    [TOKEN_EOF]     = Shift(10),
    [TOKEN_NUMBER]  = ParseError,
    [TOKEN_ID]      = ParseError,
    [TOKEN_LPAREN]  = ParseError,
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [5] = { // expr: product .
    [TOKEN_EOF]     = Reduce(1),
    [TOKEN_NUMBER]  = Reduce(1),
    [TOKEN_ID]      = Reduce(1),
    [TOKEN_LPAREN]  = Reduce(1),
    [TOKEN_RPAREN]  = Reduce(1),
    [TOKEN_STAR]    = Shift(11),
    [TOKEN_SLASH]   = Shift(12),
    [TOKEN_MINUS]   = Reduce(1),
    [TOKEN_PLUS]    = Reduce(1),
  },
  [6] = { // product: sum .
    [TOKEN_EOF]     = Reduce(5),
    [TOKEN_NUMBER]  = Reduce(5),
    [TOKEN_ID]      = Reduce(5),
    [TOKEN_LPAREN]  = Reduce(5),
    [TOKEN_RPAREN]  = Reduce(5),
    [TOKEN_STAR]    = Reduce(5),
    [TOKEN_SLASH]   = Reduce(5),
    [TOKEN_MINUS]   = Shift(14),
    [TOKEN_PLUS]    = Shift(13),
  },
  [7] = { // sum: primary .
    [TOKEN_EOF]     = Reduce(8),
    [TOKEN_NUMBER]  = Reduce(8),
    [TOKEN_ID]      = Reduce(8),
    [TOKEN_LPAREN]  = Reduce(8),
    [TOKEN_RPAREN]  = Reduce(8),
    [TOKEN_STAR]    = Reduce(8),
    [TOKEN_SLASH]   = Reduce(8),
    [TOKEN_MINUS]   = Reduce(8),
    [TOKEN_PLUS]    = Reduce(8),
  },
  [8] = { // primary: value .
    [TOKEN_EOF]     = Reduce(9),
    [TOKEN_NUMBER]  = Reduce(9),
    [TOKEN_ID]      = Reduce(9),
    [TOKEN_LPAREN]  = Reduce(9),
    [TOKEN_RPAREN]  = Reduce(9),
    [TOKEN_STAR]    = Reduce(9),
    [TOKEN_SLASH]   = Reduce(9),
    [TOKEN_MINUS]   = Reduce(9),
    [TOKEN_PLUS]    = Reduce(9),
  },
  [9] = { // primary: '(' expr . ')'
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = ParseError,
    [TOKEN_ID]      = ParseError,
    [TOKEN_LPAREN]  = ParseError,
    [TOKEN_RPAREN]  = Shift(15),
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [10] = { // $accept: expr $end .
    [TOKEN_EOF]     = ParseStop,
    [TOKEN_NUMBER]  = ParseStop,
    [TOKEN_ID]      = ParseStop,
    [TOKEN_LPAREN]  = ParseStop,
    [TOKEN_RPAREN]  = ParseStop,
    [TOKEN_STAR]    = ParseStop,
    [TOKEN_SLASH]   = ParseStop,
    [TOKEN_MINUS]   = ParseStop,
    [TOKEN_PLUS]    = ParseStop,
  },
  [11] = { // product: product '*' . sum
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [12] = { // product: product '/' . sum
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [13] = { // sum: sum '+' . primary
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [14] = { // sum: sum '-' . primary
    [TOKEN_EOF]     = ParseError,
    [TOKEN_NUMBER]  = Shift(2),
    [TOKEN_ID]      = Shift(1),
    [TOKEN_LPAREN]  = Shift(3),
    [TOKEN_RPAREN]  = ParseError,
    [TOKEN_STAR]    = ParseError,
    [TOKEN_SLASH]   = ParseError,
    [TOKEN_MINUS]   = ParseError,
    [TOKEN_PLUS]    = ParseError,
  },
  [15] = { // primary: '(' expr ')' .
    [TOKEN_EOF]     = Reduce(10),
    [TOKEN_NUMBER]  = Reduce(10),
    [TOKEN_ID]      = Reduce(10),
    [TOKEN_LPAREN]  = Reduce(10),
    [TOKEN_RPAREN]  = Reduce(10),
    [TOKEN_STAR]    = Reduce(10),
    [TOKEN_SLASH]   = Reduce(10),
    [TOKEN_MINUS]   = Reduce(10),
    [TOKEN_PLUS]    = Reduce(10),
  },
  [16] = { // product: product '*' sum .
    [TOKEN_EOF]     = Reduce(3),
    [TOKEN_NUMBER]  = Reduce(3),
    [TOKEN_ID]      = Reduce(3),
    [TOKEN_LPAREN]  = Reduce(3),
    [TOKEN_RPAREN]  = Reduce(3),
    [TOKEN_STAR]    = Reduce(3),
    [TOKEN_SLASH]   = Reduce(3),
    [TOKEN_MINUS]   = Shift(14),
    [TOKEN_PLUS]    = Shift(13),
  },
  [17] = { // product: product '/' sum .
    [TOKEN_EOF]     = Reduce(4),
    [TOKEN_NUMBER]  = Reduce(4),
    [TOKEN_ID]      = Reduce(4),
    [TOKEN_LPAREN]  = Reduce(4),
    [TOKEN_RPAREN]  = Reduce(4),
    [TOKEN_STAR]    = Reduce(4),
    [TOKEN_SLASH]   = Reduce(4),
    [TOKEN_MINUS]   = Shift(14),
    [TOKEN_PLUS]    = Shift(13),
  },
  [18] = { // sum: sum '+' primary .
    [TOKEN_EOF]     = Reduce(6),
    [TOKEN_NUMBER]  = Reduce(6),
    [TOKEN_ID]      = Reduce(6),
    [TOKEN_LPAREN]  = Reduce(6),
    [TOKEN_RPAREN]  = Reduce(6),
    [TOKEN_STAR]    = Reduce(6),
    [TOKEN_SLASH]   = Reduce(6),
    [TOKEN_MINUS]   = Reduce(6),
    [TOKEN_PLUS]    = Reduce(6),
  },
  [19] = { // sum: sum '-' primary .
    [TOKEN_EOF]     = Reduce(7),
    [TOKEN_NUMBER]  = Reduce(7),
    [TOKEN_ID]      = Reduce(7),
    [TOKEN_LPAREN]  = Reduce(7),
    [TOKEN_RPAREN]  = Reduce(7),
    [TOKEN_STAR]    = Reduce(7),
    [TOKEN_SLASH]   = Reduce(7),
    [TOKEN_MINUS]   = Reduce(7),
    [TOKEN_PLUS]    = Reduce(7),
  },
};

static char *state_descriptions[] = {
  [0]   = "S → · expr $",
  [1]   = "value → id ·",
  [2]   = "value → num ·",
  [3]   = "primary → ( · expr )",
  [4]   = "S → expr · $",
  [5]   = "exp → product ·",
  [6]   = "product → sum ·",
  [7]   = "sum → primary ·",
  [8]   = "primary → value ·",
  [9]   = "primary → ( expr · )",
  [10]  = "S → expr $ ·",
  [11]  = "product → product * · sum",
  [12]  = "product → product / · sum",
  [13]  = "sum → sum + · primary",
  [14]  = "sum → sum - · primary",
  [15]  = "primary → ( expr ) ·",
  [16]  = "product → product * sum ·",
  [17]  = "product → product / sum ·",
  [18]  = "sum → sum + primary ·",
  [19]  = "sum → sum - primary ·",
};

typedef struct {
  u32 num;
  ParseSymbol result;
} ReduceRule;

static ReduceRule reduce[] = {
  [0]   = {0, Expr},
  [1]   = {1, Expr},
  [2]   = {0, Expr},
  [3]   = {3, Product},
  [4]   = {3, Product},
  [5]   = {1, Product},
  [6]   = {3, Sum},
  [7]   = {3, Sum},
  [8]   = {1, Sum},
  [9]   = {1, Primary},
  [10]  = {3, Primary},
  [11]  = {1, Value},
  [12]  = {1, Value},
};

static u32 goto_table[][NUM_PARSE_SYM] = {
  [0] = {
    [Expr]    = 4,
    [Product] = 5,
    [Sum]     = 6,
    [Primary] = 7,
    [Value]   = 8
  },
  [3] = {
    [Expr]    = 9,
    [Product] = 5,
    [Sum]     = 6,
    [Primary] = 7,
    [Value]   = 8
  },
  [11] = {
    [Sum]     = 16,
    [Primary] = 7,
    [Value]   = 8
  },
  [12] = {
    [Sum]     = 17,
    [Primary] = 7,
    [Value]   = 8
  },
  [13] = {
    [Primary] = 18,
    [Value]   = 8
  },
  [14] = {
    [Primary] = 19,
    [Value]   = 8
  }
};

static void ShiftState(Parser *p, u32 state_id)
{
  printf("Shift %d\n", state_id);
  ParseState next_state = {state_id, p->next_token};
  VecPush(p->stack, next_state);
  p->next_token = NextToken(&p->lex);
}

static bool ReduceState(Parser *p, u32 rule_id)
{
  ReduceRule rule = reduce[rule_id];

  // ASTNode *node = malloc(sizeof(ASTNode));
  // *node = (ASTNode){rule.result, NULL};
  printf("(%d, %d)\n", VecCount(p->stack), rule.num);
  printf("Reducing to %s: ", ParseSymbolStr(rule.result));
  for (u32 i = 0; i < rule.num; i++) {
    printf("%d ", VecPeek(p->stack, i).id);
  }
  printf("\n");

  RewindVec(p->stack, rule.num);

  ParseState prev_state = VecLast(p->stack);

  ParseState next_state = {
    goto_table[prev_state.id][rule.result],
    {TOKEN_EOF, 1, 1, p->lex.src, 0}
  };

  if (next_state.id == 0) {
    printf("Undefined goto for state %d, terminal %d\n", prev_state.id, rule.result);
    return false;
  }

  printf("    Goto %d\n", next_state.id);

  VecPush(p->stack, next_state);
  return true;
}

bool ParseNext(Parser *p)
{
  ParseState state = VecLast(p->stack);
  u32 action = actions[state.id][p->next_token.type];

  for (u32 i = 0; i < VecCount(p->stack); i++) {
    printf("%d[", p->stack[i].id);
    PrintToken(p->stack[i].token);
    printf("] ");
  }
  printf("    ");
  PrintToken(p->next_token);
  printf("\n    ");
  printf("%s\n    ", state_descriptions[state.id]);

  if (ActionFor(action) == ParseStop) {
    printf("Accept\n");
    return true;
  }

  if (ActionFor(action) == ParseShift) {
    ShiftState(p, ActionVal(action));
    return ParseNext(p);
  }

  if (ActionFor(action) == ParseReduce) {
    if (!ReduceState(p, ActionVal(action))) return false;
    return ParseNext(p);
  }

  printf("Error at %d:%d\n", p->lex.line, p->lex.col);

  return false;
}

bool Parse(char *src)
{
  Parser p;
  InitLexer(&p.lex, src);
  p.next_token = NextToken(&p.lex);

  p.stack = NULL;
  ParseState initial_state = {0, {TOKEN_EOF, 1, 1, src, 0}};
  VecPush(p.stack, initial_state);
  return ParseNext(&p);
}
