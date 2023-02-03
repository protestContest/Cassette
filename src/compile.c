#include "compile.h"
#include "chunk.h"
#include "mem.h"
#include "scan.h"
#include "vec.h"

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
  PREC_NEGATIVE,
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

ParseRule rules[] = {
  [TOKEN_LPAREN] =      { Grouping,         NULL,           PREC_NONE     },
  [TOKEN_RPAREN] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACKET] =    { List,             NULL,           PREC_NONE     },
  [TOKEN_RBRACKET] =    { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACE] =      { Dict,             NULL,           PREC_NONE     },
  [TOKEN_RBRACE] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_COMMA] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_DOT] =         { NULL,             Operator,       PREC_ACCESS   },
  [TOKEN_MINUS] =       { Unary,            Operator,       PREC_TERM     },
  [TOKEN_PLUS] =        { Variable,         Operator,       PREC_TERM     },
  [TOKEN_STAR] =        { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_SLASH] =       { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_BAR] =         { NULL,             Operator,       PREC_NONE     },
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
  [TOKEN_AND] =         { Variable,         NULL,           PREC_NONE     },
  [TOKEN_NOT] =         { Unary,            NULL,           PREC_NONE     },
  [TOKEN_OR] =          { Variable,         NULL,           PREC_NONE     },
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

static bool ParseLevel(Parser *p, Precedence prec)
{
  printf("%*s== Parse Level %s == \n", 2*indent, "", PrecStr(prec));
  PrintParser(p);

  indent++;
  while (CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

  ParseRule *rule = GetRule(p);
  if (rule->prefix == NULL) {
    indent--;
    return false;
  }

  rule->prefix(p);
  while (CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

  rule = GetRule(p);
  while (rule->prec >= prec) {
    printf("Infix %s %.*s\n", TokenStr(p->token.type), p->token.length, p->token.lexeme);
    rule->infix(p);
    while (CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);

    rule = GetRule(p);
  }

  Disassemble(PrecStr(prec), p->chunk);

  indent--;
  return true;
}

static bool HasPrefix(Parser *p)
{
  ParseRule *rule = GetRule(p);
  return rule->prefix != NULL;
}

static void Grouping(Parser *p)
{
  Parser saved = *p;
  u32 start = ChunkSize(p->chunk);

  ExpectToken(p, TOKEN_LPAREN);
  if (!CompileExpr(p)) {
    PutInst(p->chunk, OP_CALL);
  }
  ExpectToken(p, TOKEN_RPAREN);

  Disassemble("End Group", p->chunk);

  if (CurToken(p) == TOKEN_ARROW) {
    *p = saved;
    RewindVec(p->chunk->code, ChunkSize(p->chunk) - start);
    Disassemble("Lambda detected", p->chunk);
    PrintParser(p);
    Lambda(p);
  }
}

static void Lambda(Parser *p)
{
  // params
  u32 num_params = 0;
  ExpectToken(p, TOKEN_LPAREN);
  while (!MatchToken(p, TOKEN_RPAREN)) {
    num_params++;
    Sym(p);
    MatchToken(p, TOKEN_COMMA);
  }
  ExpectToken(p, TOKEN_ARROW);

  Disassemble("Lambda params", p->chunk);

  // temporary jump inst
  PutInst(p->chunk, OP_JUMP, 0);
  u32 start = ChunkSize(p->chunk);

  // compile body
  CompileExpr(p);
  if (VecLast(p->chunk->code) == OP_CALL) {
    SetByte(p->chunk, ChunkSize(p->chunk) - 1, OP_APPLY);
  } else {
    PutInst(p->chunk, OP_RETURN);
  }

  // patch jump inst
  SetByte(p->chunk, start - 1, ChunkSize(p->chunk) - start);

  // define lambda (params, position on stack)
  PutInst(p->chunk, OP_CONST, IntVal(start));
  PutInst(p->chunk, OP_LAMBDA, num_params);
}

static void List(Parser *p)
{
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
  ExpectToken(p, TOKEN_LBRACE);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    Sym(p);
    ExpectToken(p, TOKEN_COLON);
    CompileExpr(p);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->chunk, OP_DICT, num);
}

static void Unary(Parser *p)
{
  TokenType op = CurToken(p);
  AdvanceToken(p);
  ExpectToken(p, TOKEN_MINUS);
  if (!ParseLevel(p, PREC_NEGATIVE)) {
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
  Sym(p);
  PutInst(p->chunk, OP_LOOKUP);
}

static void String(Parser *p)
{
  Val bin = MakeBinary(p->chunk->constants, p->token.lexeme, p->token.length);
  PutInst(p->chunk, OP_CONST, PutConst(p->chunk, bin));
  AdvanceToken(p);
}

static void Number(Parser *p)
{
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
  ExpectToken(p, TOKEN_DEF);
  Sym(p);
  CompileExpr(p);
  PutInst(p->chunk, OP_DEFINE);
}

static void Cond(Parser *p)
{

}

static void If(Parser *p)
{
  ExpectToken(p, TOKEN_IF);

  // condition
  CompileSubExpr(p);

  PutInst(p->chunk, OP_NOT);
  PutInst(p->chunk, OP_BRANCH, 0);
  u32 true_branch = ChunkSize(p->chunk);

  // true branch
  CompileSubExpr(p);
  PutInst(p->chunk, OP_JUMP, 0);
  u32 false_branch = ChunkSize(p->chunk);

  // false branch
  SetByte(p->chunk, true_branch - 1, false_branch - true_branch);
  CompileSubExpr(p);

  // after condition
  SetByte(p->chunk, false_branch - 1, ChunkSize(p->chunk) - false_branch);
}

static void Script(Parser *p)
{
  while (!MatchToken(p, TOKEN_EOF)) {
    CompileExpr(p);
    MatchToken(p, TOKEN_COMMA);
    PutInst(p->chunk, OP_POP);
  }
  RewindVec(p->chunk->code, 1);
}

static void Do(Parser *p)
{
  ExpectToken(p, TOKEN_DO);
  while (!MatchToken(p, TOKEN_END)) {
    CompileExpr(p);
    MatchToken(p, TOKEN_COMMA);
    PutInst(p->chunk, OP_POP);
  }
  RewindVec(p->chunk->code, 1);
}

static void Sym(Parser *p)
{
  Val sym = PutSymbol(p->chunk, p->token.lexeme, p->token.length);
  PutInst(p->chunk, OP_CONST, sym);
  AdvanceToken(p);
}

static void Literal(Parser *p)
{
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
}

static void Operator(Parser *p)
{
  TokenType op = CurToken(p);
  ParseRule *rule = GetRule(p);

  AdvanceToken(p);
  ParseLevel(p, rule->prec + 1);

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

static bool CompileSubExpr(Parser *p)
{
  return ParseLevel(p, PREC_EXPR + 1);
}

static bool CompileExpr(Parser *p)
{
  // empty expression, pretend we handled calling it
  if (!HasPrefix(p)) return true;

  // compile first sub-expr (the operator)
  u32 start = ChunkSize(p->chunk);
  CompileSubExpr(p);

  // if it's alone, don't call it
  if (!HasPrefix(p)) {
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
  case PREC_NEGATIVE: return "NEGATIVE";
  case PREC_ACCESS:   return "ACCESS";
  case PREC_PRIMARY:  return "PRIMARY";
  }
}
