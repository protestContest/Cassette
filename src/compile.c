#include "compile.h"
#include "chunk.h"
#include "mem.h"
#include "scan.h"
#include "vec.h"

#define DEBUG_COMPILE 1

typedef enum {
  PREC_NONE,
  PREC_BLOCK,
  PREC_PIPE,
  PREC_EXPR,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_PAIR,
  PREC_TERM,
  PREC_FACTOR,
  PREC_EXPONENT,
  PREC_LAMBDA,
  PREC_UNARY,
  PREC_ACCESS,
  PREC_PRIMARY,
} Precedence;

typedef void(*ParseFn)(Parser *p);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
} ParseRule;

static char *PrecStr(Precedence prec);
static bool CompileSubExpr(Parser *p);
static bool CompileExpr(Parser *p);
static void ConsumeSymbol(Parser *p);

static void Grouping(Parser *p);
static void Lambda(Parser *p);
static void List(Parser *p);
static void Dict(Parser *p);
static void Unary(Parser *p);
static void Variable(Parser *p);
static void String(Parser *p);
static void Number(Parser *p);
static void Define(Parser *p);
static void Cond(Parser *p);
static void If(Parser *p);
static void Do(Parser *p);
static void Sym(Parser *p);
static void Literal(Parser *p);
static void Operator(Parser *p);
static void Access(Parser *p);
static void Logic(Parser *p);

ParseRule rules[] = {
  [TOKEN_LPAREN] =      { Grouping,         NULL,           PREC_NONE     },
  [TOKEN_RPAREN] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACKET] =    { List,             NULL,           PREC_NONE     },
  [TOKEN_RBRACKET] =    { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACE] =      { Dict,             NULL,           PREC_NONE     },
  [TOKEN_RBRACE] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_COMMA] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_DOT] =         { NULL,             Access,         PREC_ACCESS   },
  [TOKEN_MINUS] =       { Unary,            Operator,       PREC_TERM     },
  [TOKEN_PLUS] =        { Variable,         Operator,       PREC_TERM     },
  [TOKEN_STAR] =        { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_SLASH] =       { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_BAR] =         { NULL,             Operator,       PREC_PAIR     },
  [TOKEN_EQ] =          { Variable,         Operator,       PREC_EQUALITY },
  [TOKEN_NEQ] =         { Variable,         Operator,       PREC_EQUALITY },
  [TOKEN_GT] =          { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_GTE] =         { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_LT] =          { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_LTE] =         { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_PIPE] =        { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ARROW] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_IDENTIFIER] =  { Variable,         NULL,           PREC_NONE     },
  [TOKEN_STRING] =      { String,           NULL,           PREC_NONE     },
  [TOKEN_NUMBER] =      { Number,           NULL,           PREC_NONE     },
  [TOKEN_AND] =         { Variable,         Logic,          PREC_LOGIC    },
  [TOKEN_OR] =          { Variable,         Logic,          PREC_LOGIC    },
  [TOKEN_NOT] =         { Unary,            NULL,           PREC_NONE     },
  [TOKEN_DEF] =         { Define,           NULL,           PREC_NONE     },
  [TOKEN_COND] =        { Cond,             NULL,           PREC_NONE     },
  [TOKEN_IF] =          { If,               NULL,           PREC_NONE     },
  [TOKEN_DO] =          { Do,               NULL,           PREC_NONE     },
  [TOKEN_ELSE] =        { NULL,             NULL,           PREC_NONE     },
  [TOKEN_END] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_TRUE] =        { Literal,          NULL,           PREC_NONE     },
  [TOKEN_FALSE] =       { Literal,          NULL,           PREC_NONE     },
  [TOKEN_NIL] =         { Literal,          NULL,           PREC_NONE     },
  [TOKEN_NEWLINE] =     { NULL,             NULL,           PREC_NONE     },
  [TOKEN_EOF] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ERROR] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_COLON] =       { Sym,              NULL,           PREC_NONE     },
};

#define GetRule(p)  (&rules[(p)->token.type])

static u32 indent = 0;

// returns whether the next token could be parsed
static bool ParseLevel(Parser *p, Precedence prec, bool skip_newlines)
{
  indent++;
  while (skip_newlines && CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

#if DEBUG_COMPILE
    printf("%-10s ", PrecStr(prec));
    for (u32 i = 0; i < indent; i++) printf("▪︎ ");
    printf("Prefix %s \"%.*s\"\n", TokenStr(CurToken(p)), p->token.length, p->token.lexeme);
    PrintSourceContext(p, 0);
#endif

  ParseRule *rule = GetRule(p);
  if (rule->prefix == NULL) {
    indent--;
    return false;
  }

  rule->prefix(p);
  while (skip_newlines && CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

  rule = GetRule(p);
  while (rule->prec >= prec) {
#if DEBUG_COMPILE
    printf("%-10s ", PrecStr(prec));
    for (u32 i = 0; i < indent; i++) printf("▪︎ ");
    printf("Infix %s \"%.*s\"\n", TokenStr(CurToken(p)), p->token.length, p->token.lexeme);
#endif

    rule->infix(p);
    while (skip_newlines && CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

    rule = GetRule(p);
  }

  indent--;
  return true;
}

static bool CanParseLevel(Parser *p)
{
  ParseRule *rule = GetRule(p);
  return rule->prefix != NULL;
}

static void Grouping(Parser *p)
{
  if (DEBUG_COMPILE) printf("Grouping\n");

  Parser saved = *p;
  u32 start = ChunkSize(p->chunk);

  ExpectToken(p, TOKEN_LPAREN);
  // TODO: replace CompileExpr call with one that skips newlines
  if (!CompileExpr(p)) {
    PutInst(p->chunk, OP_CALL);
  }
  ExpectToken(p, TOKEN_RPAREN);

  if (CurToken(p) == TOKEN_ARROW) {
    *p = saved;
    RewindVec(p->chunk->code, ChunkSize(p->chunk) - start);
    Lambda(p);
  }
}

static void Lambda(Parser *p)
{
  if (DEBUG_COMPILE) printf("Lambda\n");

  // params
  u32 num_params = 0;
  ExpectToken(p, TOKEN_LPAREN);
  while (!MatchToken(p, TOKEN_RPAREN)) {
    num_params++;
    ConsumeSymbol(p);
    MatchToken(p, TOKEN_COMMA);
  }
  ExpectToken(p, TOKEN_ARROW);

  // temporary jump inst
  PutInst(p->chunk, OP_JUMP, 0);
  u32 start = ChunkSize(p->chunk);

  // compile body
  CompileExpr(p);
  if (VecLast(p->chunk->code) == OP_CALL) {
    SetByte(p->chunk, ChunkSize(p->chunk) - 1, OP_APPLY);
  }
  PutInst(p->chunk, OP_RETURN);

  // patch jump inst
  SetByte(p->chunk, start - 1, ChunkSize(p->chunk) - start);

  // define lambda (params, position on stack)
  PutInst(p->chunk, OP_CONST, IntVal(start));
  PutInst(p->chunk, OP_LAMBDA, num_params);
}

static void List(Parser *p)
{
  if (DEBUG_COMPILE) printf("List\n");

  ExpectToken(p, TOKEN_LBRACKET);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACKET)) {
    CompileExpr(p);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->chunk, OP_LIST, num);
}

static void Dict(Parser *p)
{
  if (DEBUG_COMPILE) printf("Dict\n");

  ExpectToken(p, TOKEN_LBRACE);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    ConsumeSymbol(p);
    ExpectToken(p, TOKEN_COLON);
    CompileExpr(p);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->chunk, OP_DICT, num);
}

static void Unary(Parser *p)
{
  if (DEBUG_COMPILE) printf("Unary\n");

  TokenType op = CurToken(p);
  AdvanceToken(p);
  ExpectToken(p, TOKEN_MINUS);
  if (!ParseLevel(p, PREC_UNARY, true)) {
    // error
  }

  if (op == TOKEN_MINUS) {
    PutInst(p->chunk, OP_NEG);
  } else if (op == TOKEN_NOT) {
    PutInst(p->chunk, OP_NOT);
  } else {
    // error
  }
}

static void Variable(Parser *p)
{
  if (DEBUG_COMPILE) printf("Variable\n");

  ConsumeSymbol(p);
  PutInst(p->chunk, OP_LOOKUP);
}

static void String(Parser *p)
{
  if (DEBUG_COMPILE) printf("String\n");

  Val bin = MakeBinary(p->chunk->constants, p->token.lexeme, p->token.length);
  PutInst(p->chunk, OP_CONST, PutConst(p->chunk, bin));
  AdvanceToken(p);
}

static void Number(Parser *p)
{
  if (DEBUG_COMPILE) printf("Number\n");

  Token token = p->token;
  AdvanceToken(p);
  u32 num = 0;

  for (u32 i = 0; i < token.length; i++) {
    if (token.lexeme[i] == '_') continue;
    if (token.lexeme[i] == '.') {
      float frac = 0.0;
      float magn = 10.0;
      for (u32 j = i+1; j < token.length; j++) {
        if (token.lexeme[i] == '_') continue;
        u32 digit = token.lexeme[j] - '0';
        frac += (float)digit / magn;
      }

      float f = (float)num + frac;
      PutInst(p->chunk, OP_CONST, NumVal(f));
      return;
    }

    u32 digit = token.lexeme[i] - '0';
    num = num*10 + digit;
  }

  if (num == 0) {
    PutInst(p->chunk, OP_ZERO);
  } else {
    PutInst(p->chunk, OP_CONST, IntVal(num));
  }
}

static void Define(Parser *p)
{
  if (DEBUG_COMPILE) printf("Define\n");

  ExpectToken(p, TOKEN_DEF);
  ConsumeSymbol(p);
  CompileSubExpr(p);
  PutInst(p->chunk, OP_DEFINE);
}

static void Cond(Parser *p)
{
  // TODO
}

static void If(Parser *p)
{
  if (DEBUG_COMPILE) printf("If\n");

  ExpectToken(p, TOKEN_IF);

  // condition
  CompileSubExpr(p);
  SkipNewlines(p);
  PutInst(p->chunk, OP_NOT);
  PutInst(p->chunk, OP_BRANCH, 0);
  u32 true_branch = ChunkSize(p->chunk);

  // true branch
  CompileSubExpr(p);
  SkipNewlines(p);
  PutInst(p->chunk, OP_JUMP, 0);
  u32 false_branch = ChunkSize(p->chunk);

  // false branch
  SetByte(p->chunk, true_branch - 1, false_branch - true_branch);
  CompileSubExpr(p);
  SkipNewlines(p);

  // after condition
  SetByte(p->chunk, false_branch - 1, ChunkSize(p->chunk) - false_branch);
}

static void Script(Parser *p)
{
  if (DEBUG_COMPILE) printf("Script\n");

  while (!MatchToken(p, TOKEN_EOF)) {
    CompileExpr(p);
    SkipNewlines(p);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }
  RewindVec(p->chunk->code, 1);
}

static void Do(Parser *p)
{
  if (DEBUG_COMPILE) printf("Do\n");

  ExpectToken(p, TOKEN_DO);
  while (!MatchToken(p, TOKEN_END)) {
    CompileExpr(p);
    SkipNewlines(p);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }
  RewindVec(p->chunk->code, 1);
}

static void Sym(Parser *p)
{
  if (DEBUG_COMPILE) printf("Symbol\n");

  ExpectToken(p, TOKEN_COLON);
  ConsumeSymbol(p);
}

static void ConsumeSymbol(Parser *p)
{
  Val sym = PutSymbol(p->chunk, p->token.lexeme, p->token.length);
  PutInst(p->chunk, OP_CONST, sym);
  AdvanceToken(p);
}

static void Literal(Parser *p)
{
  if (DEBUG_COMPILE) printf("Literal\n");

  switch (CurToken(p)) {
  case TOKEN_TRUE:
    PutInst(p->chunk, OP_TRUE);
    break;
  case TOKEN_FALSE:
    PutInst(p->chunk, OP_FALSE);
    break;
  case TOKEN_NIL:
    PutInst(p->chunk, OP_NIL);
    break;
  default:
    // error
    break;
  }
  AdvanceToken(p);
}

static void Operator(Parser *p)
{
  if (DEBUG_COMPILE) printf("Operator\n");

  TokenType op = CurToken(p);
  ParseRule *rule = GetRule(p);

  AdvanceToken(p);
  ParseLevel(p, rule->prec + 1, true);

  switch (op) {
  case TOKEN_PLUS:
    PutInst(p->chunk, OP_ADD);
    break;
  case TOKEN_MINUS:
    PutInst(p->chunk, OP_SUB);
    break;
  case TOKEN_STAR:
    PutInst(p->chunk, OP_MUL);
    break;
  case TOKEN_SLASH:
    PutInst(p->chunk, OP_DIV);
    break;
  case TOKEN_BAR:
    PutInst(p->chunk, OP_PAIR);
    break;
  case TOKEN_EQ:
    PutInst(p->chunk, OP_EQUAL);
    break;
  case TOKEN_NEQ:
    PutInst(p->chunk, OP_EQUAL);
    PutInst(p->chunk, OP_NOT);
    break;
  case TOKEN_GT:
    PutInst(p->chunk, OP_GT);
    break;
  case TOKEN_GTE:
    PutInst(p->chunk, OP_LT);
    PutInst(p->chunk, OP_NOT);
    break;
  case TOKEN_LT:
    PutInst(p->chunk, OP_LT);
    break;
  case TOKEN_LTE:
    PutInst(p->chunk, OP_GT);
    PutInst(p->chunk, OP_NOT);
    break;
  default:
    // error
    break;
  }
}

static void Access(Parser *p)
{
  if (DEBUG_COMPILE) printf("Access\n");

  ExpectToken(p, TOKEN_DOT);
  ConsumeSymbol(p);
  PutInst(p->chunk, OP_ACCESS);
}

static void Logic(Parser *p)
{
  if (DEBUG_COMPILE) printf("Logic\n");

  if (CurToken(p) == TOKEN_AND) {
    PutInst(p->chunk, OP_NOT);
  }

  // duplicate stack top, so our branch doesn't consume it
  PutInst(p->chunk, OP_DUP);
  PutInst(p->chunk, OP_BRANCH, 0);
  u32 branch = ChunkSize(p->chunk);

  // if we get to the second clause, we can discard the result from the first
  PutInst(p->chunk, OP_POP);

  ParseRule *rule = GetRule(p);
  AdvanceToken(p);
  ParseLevel(p, rule->prec + 1, true);

  SetByte(p->chunk, branch - 1, ChunkSize(p->chunk) - branch);
}

// returns whether the next token could be parsed
static bool CompileSubExpr(Parser *p)
{
  return ParseLevel(p, PREC_EXPR + 1, false);
}

// returns whether the expression was called
static bool CompileExpr(Parser *p)
{
  if (DEBUG_COMPILE) printf("Expression\n");

  // empty expression, pretend we handled calling it
  if (!CanParseLevel(p)) {
    return true;
  }

  // compile first sub-expr (the operator)
  u32 start = ChunkSize(p->chunk);
  CompileSubExpr(p);

  // if it's alone, don't call it
  if (!CanParseLevel(p)) {
    return false;
  }

  // copy operator code out
  u32 length = ChunkSize(p->chunk) - start;
  u8 operator_code[length];
  memcpy(operator_code, &p->chunk->code[start], length);
  RewindVec(p->chunk->code, length);

  // compile the rest of the expr
  while (true) {
    if (!CompileSubExpr(p)) break;
  }

  // copy the operator code to the end
  start = ChunkSize(p->chunk);
  GrowVec(p->chunk->code, length);
  memcpy(&p->chunk->code[start], operator_code, length);

  PutInst(p->chunk, OP_CALL);
  return true;
}

Status Compile(char *src, Chunk *chunk)
{
  Parser p;
  InitParser(&p, src, chunk);

  AdvanceToken(&p);
  Script(&p);

  return Ok;
}

static char *PrecStr(Precedence prec)
{
  switch (prec)
  {
  case PREC_NONE:     return "NONE";
  case PREC_BLOCK:    return "BLOCK";
  case PREC_PIPE:     return "PIPE";
  case PREC_EXPR:     return "EXPR";
  case PREC_LOGIC:    return "LOGIC";
  case PREC_EQUALITY: return "EQUALITY";
  case PREC_COMPARE:  return "COMPARE";
  case PREC_PAIR:     return "PAIR";
  case PREC_TERM:     return "TERM";
  case PREC_FACTOR:   return "FACTOR";
  case PREC_EXPONENT: return "EXPONENT";
  case PREC_LAMBDA:   return "LAMBDA";
  case PREC_UNARY:    return "UNARY";
  case PREC_ACCESS:   return "ACCESS";
  case PREC_PRIMARY:  return "PRIMARY";
  }
}
