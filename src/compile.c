#include "compile.h"
#include "chunk.h"

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

static const char *PrecStr(Precedence prec)
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

typedef void (*ParseFn)(Parser *p);

static void ParseNumber(Parser *p);
static void ParseIdentifier(Parser *p);
static void ParseNegative(Parser *p);
static void ParseLiteral(Parser *p);
static void ParseOperator(Parser *p);
static void ParseGroup(Parser *p);
static void ParseExpr(Parser *p);
static bool ParseLevel(Parser *p, Precedence level);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
} ParseRule;

ParseRule rules[] = {
  [TOKEN_LPAREN] =      { ParseGroup,       ParseExpr,      PREC_EXPR     },
  [TOKEN_RPAREN] =      { NULL,             NULL,           PREC_NONE     },
  // [TOKEN_LBRACKET] =    { ParseList,        NULL,           PREC_NONE     },
  [TOKEN_RBRACKET] =    { NULL,             NULL,           PREC_NONE     },
  // [TOKEN_LBRACE] =      { ParseDict,        ParseExpr,      PREC_EXPR     },
  [TOKEN_RBRACE] =      { NULL,             NULL,           PREC_NONE     },
  // [TOKEN_COMMA] =       { NULL,             ParseBlock,     PREC_BLOCK    },
  // [TOKEN_DOT] =         { NULL,             ParseAccess,    PREC_ACCESS   },
  [TOKEN_MINUS] =       { ParseNegative,    ParseOperator,  PREC_TERM     },
  [TOKEN_PLUS] =        { ParseIdentifier,  ParseOperator,  PREC_TERM     },
  [TOKEN_STAR] =        { ParseIdentifier,  ParseOperator,  PREC_FACTOR   },
  [TOKEN_SLASH] =       { ParseIdentifier,  ParseOperator,  PREC_FACTOR   },
  [TOKEN_EXPONENT] =    { ParseIdentifier,  ParseOperator,  PREC_EXPONENT },
  [TOKEN_BAR] =         { NULL,             ParseOperator,  PREC_PAIR     },
  [TOKEN_EQ] =          { ParseIdentifier,  ParseOperator,  PREC_EQUALITY },
  [TOKEN_NEQ] =         { ParseIdentifier,  ParseOperator,  PREC_EQUALITY },
  [TOKEN_GT] =          { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_GTE] =         { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_LT] =          { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_LTE] =         { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  // [TOKEN_PIPE] =        { ParseIdentifier,  ParsePipe,      PREC_PIPE     },
  // [TOKEN_ARROW] =       { NULL,             ParseLambda,    PREC_LAMBDA   },
  [TOKEN_IDENTIFIER] =  { ParseIdentifier,  ParseExpr,      PREC_EXPR     },
  // [TOKEN_STRING] =      { ParseString,      NULL,           PREC_NONE     },
  [TOKEN_NUMBER] =      { ParseNumber,      ParseExpr,      PREC_EXPR     },
  // [TOKEN_SYMBOL] =      { ParseSymbol,      NULL,           PREC_NONE     },
  [TOKEN_AND] =         { ParseIdentifier,  ParseOperator,  PREC_LOGIC    },
  [TOKEN_NOT] =         { ParseIdentifier,  ParseOperator,  PREC_LOGIC    },
  [TOKEN_OR] =          { ParseIdentifier,  ParseOperator,  PREC_LOGIC    },
  // [TOKEN_DEF] =         { ParseDef,         NULL,           PREC_NONE     },
  // [TOKEN_COND] =        { ParseCond,        NULL,           PREC_NONE     },
  // [TOKEN_DO] =          { ParseDo,          NULL,           PREC_NONE     },
  // [TOKEN_ELSE] =        { NULL,             ParseElse,      PREC_EXPR     },
  [TOKEN_END] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_TRUE] =        { ParseLiteral,     NULL,           PREC_NONE     },
  [TOKEN_FALSE] =       { ParseLiteral,     NULL,           PREC_NONE     },
  [TOKEN_NIL] =         { ParseLiteral,     NULL,           PREC_NONE     },
  // [TOKEN_NEWLINE] =     { NULL,             ParseBlock,     PREC_BLOCK    },
  [TOKEN_EOF] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ERROR] =       { NULL,             NULL,           PREC_NONE     },
};

#define CurType(p)  ((p)->current.type)
#define GetRule(p)  (&rules[CurType(p)])

void Emit(Parser *p, u8 byte)
{
  PutByte(p->chunk, p->r.line, byte);
}

void EmitConst(Parser *p, Val value)
{
  u8 i = PutConst(p->chunk, value);
  PutByte(p->chunk, p->r.line, OP_CONST);
  PutByte(p->chunk, p->r.line, i);
}

static Status SyntaxError(Parser *p, const char *message)
{
  if (p->r.status != Ok) return Error;

  fprintf(stderr, "[%d:%d] Syntax error: %s\n", p->current.line, p->current.col, message);
  PrintSourceContext(&p->r, 0);

  return Error;
}

static void Advance(Parser *p)
{
  p->current = ScanToken(&p->r);
  // printf("Advance \"%.*s\" %s\n", p->current.length, p->current.lexeme, TokenStr(p->current.type));
}

static void Expect(Parser *p, TokenType type, const char *msg)
{
  Token token = ScanToken(&p->r);

  if (token.type == type) {
    p->current = token;
  } else {
    SyntaxError(p, msg);
  }
}

static void ParseNegative(Parser *p)
{
  ParseLevel(p, PREC_NEGATIVE);
  Emit(p, OP_NEG);
}

static void ParseLiteral(Parser *p)
{
  switch (p->current.type) {
  case TOKEN_TRUE:
    Emit(p, OP_TRUE);
    break;
  case TOKEN_FALSE:
    Emit(p, OP_FALSE);
    break;
  case TOKEN_NIL:
    Emit(p, OP_NIL);
    break;
  default:  return;
  }
}

static void ParseOperator(Parser *p)
{
  TokenType op = CurType(p);
  ParseRule *rule = GetRule(p);
  ParseLevel(p, rule->prec + 1);

  switch (op) {
  case TOKEN_MINUS:
    Emit(p, OP_SUB);
    break;
  case TOKEN_PLUS:
    Emit(p, OP_ADD);
    break;
  case TOKEN_STAR:
    Emit(p, OP_MUL);
    break;
  case TOKEN_SLASH:
    Emit(p, OP_DIV);
    break;
  case TOKEN_EXPONENT:
    Emit(p, OP_EXP);
    break;
  case TOKEN_BAR:
    Emit(p, OP_CONS);
    break;
  case TOKEN_EQ:
    Emit(p, OP_EQUAL);
    break;
  case TOKEN_NEQ:
    Emit(p, OP_EQUAL);
    Emit(p, OP_NOT);
    break;
  case TOKEN_GT:
    Emit(p, OP_GT);
    break;
  case TOKEN_GTE:
    Emit(p, OP_LT);
    Emit(p, OP_NOT);
    break;
  case TOKEN_LT:
    Emit(p, OP_LT);
    break;
  case TOKEN_LTE:
    Emit(p, OP_GT);
    Emit(p, OP_NOT);
    break;
  default:
    return;
  }
}

static void ParseGroup(Parser *p)
{
  ParseExpr(p);
  Expect(p, TOKEN_RPAREN, "Expected \")\" after expression");
}

// static void ParseExpr(Parser *p)
// {
//   Token token = p->current;

//   Advance(p);
//   while (CurType(p) != TOKEN_COMMA && CurType(p) != TOKEN_NEWLINE && CurType(p) != TOKEN_EOF) {
//     if (!ParseLevel(p, PREC_EXPR + 1)) break;
//     Advance(p);
//   }
//   if (p->r.status != Ok) return;


// }

static void ParseExpr(Parser *p)
{
  ParseLevel(p, PREC_EXPR + 1);
}

static bool ParseLevel(Parser *p, Precedence level)
{
  // printf("ParseLevel %s\n", PrecStr(level));
  Advance(p);

  ParseRule *rule = GetRule(p);
  if (!rule->prefix) return false;

  rule->prefix(p);
  if (p->r.status != Ok) return false;

  Advance(p);
  rule = GetRule(p);
  while (level <= rule->prec) {
    rule->infix(p);
    if (p->r.status != Ok) return false;
    Advance(p);
    rule = GetRule(p);
  }

  return true;
}

static void ParseNumber(Parser *p)
{
  Token token = p->current;
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

      EmitConst(p, NumVal((float)num + frac));
      return;
    }

    u32 digit = token.lexeme[i] - '0';
    num = num*10 + digit;
  }

  EmitConst(p, IntVal(num));
}

static void ParseIdentifier(Parser *p)
{

}

Status Compile(char *src, Chunk *chunk)
{
  Parser p;
  InitReader(&p.r, src);
  p.chunk = chunk;

  ParseExpr(&p);

  Emit(&p, OP_RETURN);
  return p.r.status;
}
