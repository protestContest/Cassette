#include "compile.h"
#include <univ/vec.h>
#include <univ/io.h>

#define DEBUG_COMPILE

static void InitParser(Parser *p, char *src, Image *image)
{
  p->status = Ok;
  p->source.line = 1;
  p->source.col = 1;
  p->source.data = src;
  p->source.cur = 0;
  p->error = NULL;
  p->image = image;
  p->module = NULL;
}

typedef enum Precedence {
  PREC_NONE,
  PREC_EXPR,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_PAIR,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_ACCESS,
} Precedence;

typedef bool (*ParseFn)(Parser *p, bool tail);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
} ParseRule;

static void Script(Parser *p);
static bool Mod(Parser *p, bool tail);
static i32 Expr(Parser *p, bool tail);
static bool Subexpr(Parser *p, Precedence prec, bool tail);
static bool Block(Parser *p, bool tail);

static bool CanParse(Parser *p);
static bool IsSpecialForm(Parser *p);
static void ConsumeSymbol(Parser *p);
static char *PrecStr(Precedence prec);

static bool Grouping(Parser *p, bool tail);
static bool Lambda(Parser *p, bool tail);
static bool List(Parser *p, bool tail);
static bool Dict(Parser *p, bool tail);
static bool Unary(Parser *p, bool tail);
static bool Variable(Parser *p, bool tail);
static bool String(Parser *p, bool tail);
static bool Number(Parser *p, bool tail);
static bool Define(Parser *p, bool tail);
static bool Cond(Parser *p, bool tail);
static bool If(Parser *p, bool tail);
static bool Do(Parser *p, bool tail);
static bool Sym(Parser *p, bool tail);
static bool Literal(Parser *p, bool tail);
static bool Operator(Parser *p, bool tail);
static bool Access(Parser *p, bool tail);
static bool Logic(Parser *p, bool tail);
static bool Import(Parser *p, bool tail);
static bool Use(Parser *p, bool tail);
static bool Let(Parser *p, bool tail);

ParseRule rules[] = {
  [TOKEN_DOT] =         { NULL,             Access,         PREC_ACCESS   },
  [TOKEN_STAR] =        { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_SLASH] =       { Variable,         Operator,       PREC_FACTOR   },
  [TOKEN_MINUS] =       { Unary,            Operator,       PREC_TERM     },
  [TOKEN_PLUS] =        { Variable,         Operator,       PREC_TERM     },
  [TOKEN_BAR] =         { NULL,             Operator,       PREC_PAIR     },
  [TOKEN_GT] =          { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_GTE] =         { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_LT] =          { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_LTE] =         { Variable,         Operator,       PREC_COMPARE  },
  [TOKEN_EQ] =          { Variable,         Operator,       PREC_EQUALITY },
  [TOKEN_NEQ] =         { Variable,         Operator,       PREC_EQUALITY },
  [TOKEN_AND] =         { Variable,         Logic,          PREC_LOGIC    },
  [TOKEN_OR] =          { Variable,         Logic,          PREC_LOGIC    },
  [TOKEN_LPAREN] =      { Grouping,         NULL,           PREC_NONE     },
  [TOKEN_DEF] =         { Define,           NULL,           PREC_NONE     },
  [TOKEN_DO] =          { Do,               NULL,           PREC_NONE     },
  [TOKEN_IF] =          { If,               NULL,           PREC_NONE     },
  [TOKEN_COND] =        { Cond,             NULL,           PREC_NONE     },
  [TOKEN_NOT] =         { Unary,            NULL,           PREC_NONE     },
  [TOKEN_LBRACKET] =    { List,             NULL,           PREC_NONE     },
  [TOKEN_LBRACE] =      { Dict,             NULL,           PREC_NONE     },
  [TOKEN_IDENTIFIER] =  { Variable,         NULL,           PREC_NONE     },
  [TOKEN_COLON] =       { Sym,              NULL,           PREC_NONE     },
  [TOKEN_STRING] =      { String,           NULL,           PREC_NONE     },
  [TOKEN_NUMBER] =      { Number,           NULL,           PREC_NONE     },
  [TOKEN_TRUE] =        { Literal,          NULL,           PREC_NONE     },
  [TOKEN_FALSE] =       { Literal,          NULL,           PREC_NONE     },
  [TOKEN_NIL] =         { Literal,          NULL,           PREC_NONE     },
  [TOKEN_MODULE] =      { Mod,              NULL,           PREC_NONE     },
  [TOKEN_IMPORT] =      { Import,           NULL,           PREC_NONE     },
  [TOKEN_LET] =         { Let,              NULL,           PREC_NONE     },
  [TOKEN_USE] =         { Use,              NULL,           PREC_NONE     },
  [TOKEN_RPAREN] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_RBRACKET] =    { NULL,             NULL,           PREC_NONE     },
  [TOKEN_RBRACE] =      { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ELSE] =        { NULL,             NULL,           PREC_NONE     },
  [TOKEN_END] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_COMMA] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_PIPE] =        { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ARROW] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_NEWLINE] =     { NULL,             NULL,           PREC_NONE     },
  [TOKEN_EOF] =         { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ERROR] =       { NULL,             NULL,           PREC_NONE     },
};

#define GetRule(p)  (&rules[(p)->token.type])


static i32 Expr(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Expression");
  if (tail) Print("(tail)");
  Print("\n");
#endif


  Parser saved = *p;
  u32 start = ChunkSize(p->module);

  bool callable = Subexpr(p, PREC_EXPR + 1, tail);

  if (!callable) {
    return -1;
  }

  if (CanParse(p) && tail) {
#ifdef DEBUG_COMPILE
    Print("Recompiling operator\n");
#endif
    *p = saved;
    RewindVec(p->module->code, ChunkSize(p->module) - start);
    callable = Subexpr(p, PREC_EXPR + 1, false);
  }

  // parse the arguments
  i32 num_args = 0;
  while (CanParse(p)) {
    Subexpr(p, PREC_EXPR + 1, false);
    num_args++;
  }

  return num_args;
}

static bool CanParse(Parser *p)
{
  return GetRule(p)->prefix != NULL;
}

static bool IsSpecialForm(Parser *p)
{
  switch (CurToken(p)) {
  case TOKEN_IF:
  case TOKEN_DO:
  case TOKEN_DEF:
  // case TOKEN_SET:
  case TOKEN_LET:
  case TOKEN_COND:
  case TOKEN_USE:
  case TOKEN_IMPORT:
  case TOKEN_MODULE:
    return true;
  default:
    return false;
  }
}

u32 indent = 0;

static bool Subexpr(Parser *p, Precedence prec, bool tail)
{
  // if (skip_newlines) SkipNewlines(p);

#ifdef DEBUG_COMPILE
    Print("%-10s ");
    Print(PrecStr(prec));
    Print("Prefix ");
    Print(TokenStr(CurToken(p)));
    Print(" \"");
    PrintN(p->token.lexeme, p->token.length);
    Print("\"\n");
    PrintSourceContext(p, 0);
#endif

  Parser saved = *p;
  u32 start = ChunkSize(p->module);

  ParseRule *rule = GetRule(p);
  if (rule->prefix == NULL) {
    // error
    indent--;
    return false;
  }

  bool callable = rule->prefix(p, tail);
  // if (skip_newlines) SkipNewlines(p);

  rule = GetRule(p);

  if (rule->prec >= prec && tail) {
#ifdef DEBUG_COMPILE
    Print("Recompiling prefix\n");
#endif
    // recompile prefix
    *p = saved;
    RewindVec(p->module->code, ChunkSize(p->module) - start);
    GetRule(p)->prefix(p, false);
  }

  while (rule->prec >= prec) {
#ifdef DEBUG_COMPILE
    Print(PrecStr(prec));
    Print(" Infix ");
    Print(TokenStr(CurToken(p)));
    Print("\"");
    PrintN(p->token.lexeme, p->token.length);
    Print("\"\n");
#endif

    callable = rule->infix(p, false);
    // if (skip_newlines) SkipNewlines(p);

    rule = GetRule(p);
  }

  indent--;
  return callable;
}

static bool Grouping(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Grouping");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  // save state
  Parser saved = *p;
  u32 start = ChunkSize(p->module);

  ExpectToken(p, TOKEN_LPAREN);

  i32 num_args = Expr(p, tail);
  if (num_args >= 0) {
    if (tail) {
      PutInst(p->module, OP_APPLY, num_args);
    } else {
      PutInst(p->module, OP_CALL, num_args);
    }
  }

  ExpectToken(p, TOKEN_RPAREN);

  if (CurToken(p) == TOKEN_ARROW) {
    // restore state
    *p = saved;
    RewindVec(p->module->code, ChunkSize(p->module) - start);
    return Lambda(p, tail);
  } else {
    return num_args >= 0;
  }
}

Status Compile(char *src, Image *image)
{
  Parser p;
  InitParser(&p, src, image);
  p.module = CreateModule();
  PutModule(&image->modules, SymbolFor("Main"), p.module);

  // Prime the parser
  AdvanceToken(&p);
  Block(&p, false);

#ifdef DEBUG_COMPILE
  Disassemble("Script", NULL, p.module);
#endif

  return Ok;
}

Status CompileModules(char *src, Image *image)
{
  Parser p;
  InitParser(&p, src, image);

  AdvanceToken(&p);
  while (CurToken(&p) != TOKEN_EOF) {
    while (CurToken(&p) != TOKEN_MODULE && CurToken(&p) != TOKEN_EOF) AdvanceToken(&p);
    if (CurToken(&p) == TOKEN_EOF) break;
    Mod(&p, false);
  }

// #ifdef DEBUG_COMPILE
//   Disassemble("Module", chunk);
// #endif

  return Ok;
}

static bool Block(Parser *p, bool tail)
{
  Parser saved;
  u32 start;
  bool callable;

  SkipNewlines(p);
  if (!CanParse(p)) return false;

  while (CanParse(p)) {
    callable = false;
    saved = *p;
    start = ChunkSize(p->module);

    i32 num_args = Expr(p, false);
    if (num_args > 0) {
      PutInst(p->module, OP_CALL, num_args);
      callable = true;
    }

    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->module, OP_POP);
  }

  if (tail) {
#ifdef DEBUG_COMPILE
    Print("Recompiling last expr\n");
#endif

    // recompile last expr
    *p = saved;
    RewindVec(p->module->code, ChunkSize(p->module) - start);
    i32 num_args = Expr(p, true);
    if (num_args > 0) {
      PutInst(p->module, OP_APPLY, num_args);
    }

    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
  } else {
    // don't pop the final value
    RewindVec(p->module->code, 1);
  }

  return callable;
}

static u32 LambdaParams(Parser *p)
{
  u32 num_params = 0;
  while (!MatchToken(p, TOKEN_RPAREN)) {
    num_params++;
    ConsumeSymbol(p);
    MatchToken(p, TOKEN_COMMA);
  }
  return num_params;
}

static u32 LambdaBody(Parser *p)
{
  // jump past the body
  PutInst(p->module, OP_JUMP, 0);
  u32 start = ChunkSize(p->module);

  // compile body
  Subexpr(p, PREC_EXPR + 1, true);

  PutInst(p->module, OP_RETURN);

  // patch jump op
  SetByte(p->module, start - 1, ChunkSize(p->module) - start);

  return start;
}

static bool Lambda(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Lambda");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_LPAREN);
  u32 num_params = LambdaParams(p);
  ExpectToken(p, TOKEN_ARROW);
  SkipNewlines(p);
  u32 start = LambdaBody(p);

  PutInst(p->module, OP_CONST, IntVal(start));
  PutInst(p->module, OP_LAMBDA, num_params);

  return false;
}

static bool Define(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Define");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_DEF);

  if (MatchToken(p, TOKEN_LPAREN)) {
    // variable
    ConsumeSymbol(p);

    u32 num_params = LambdaParams(p);
    SkipNewlines(p);
    u32 start = LambdaBody(p);

    PutInst(p->module, OP_CONST, IntVal(start));
    PutInst(p->module, OP_LAMBDA, num_params);
  } else {
    ConsumeSymbol(p);
    Subexpr(p, PREC_EXPR + 1, false);
  }

  PutInst(p->module, OP_DEFINE);
  return false;
}

static bool Do(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Do");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_DO);
  bool callable = Block(p, tail);
  ExpectToken(p, TOKEN_END);
  return callable;
}

static bool If(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("If");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_IF);

  // condition
  Subexpr(p, PREC_EXPR + 1, false);

  // jump past the consequent if it's false
  PutInst(p->module, OP_NOT);
  PutInst(p->module, OP_BRANCH, 0);
  u32 consequent = ChunkSize(p->module);

  bool callable = false;


  // check for do-else
  bool do_else = MatchToken(p, TOKEN_DO);

  // consequent
  if (do_else) {
    callable = Block(p, tail);
  } else {
    callable = Subexpr(p, PREC_EXPR + 1, tail);
  }

  // jump past the alternative
  PutInst(p->module, OP_JUMP, 0);

  // patch jump over consequent to here
  u32 alternative = ChunkSize(p->module);
  SetByte(p->module, consequent - 1, alternative - consequent);

  // alternative (nil unless using a do/else block)
  if (do_else) {
    if (MatchToken(p, TOKEN_ELSE)) {
      callable = callable || Block(p, tail);
    } else {
      PutInst(p->module, OP_NIL);
    }
    ExpectToken(p, TOKEN_END);
  } else {
    callable = callable || Subexpr(p, PREC_EXPR + 1, tail);
  }

  // patch jump over alternative to here
  u32 after = ChunkSize(p->module);
  SetByte(p->module, alternative - 1, after - alternative);

  return callable;
}

static bool Cond(Parser *p, bool tail)
{
  // TODO
  return false;
}

static bool Unary(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Unary");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  TokenType op = CurToken(p);
  AdvanceToken(p);
  ExpectToken(p, TOKEN_MINUS);
  Subexpr(p, PREC_UNARY, false);

  if (op == TOKEN_MINUS) {
    PutInst(p->module, OP_NEG);
  } else if (op == TOKEN_NOT) {
    PutInst(p->module, OP_NOT);
  } else {
    Assert(false);
  }

  return false;
}

static bool List(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("List");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_LBRACKET);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACKET)) {
    Expr(p, false);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->module, OP_LIST, num);

  return false;
}

static bool Dict(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Dict");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_LBRACE);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    ConsumeSymbol(p);
    ExpectToken(p, TOKEN_COLON);
    Expr(p, false);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->module, OP_DICT, num);

  return false;
}

static bool Variable(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Variable");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ConsumeSymbol(p);
  PutInst(p->module, OP_LOOKUP);

  return true;
}

static bool Sym(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Symbol");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_COLON);
  ConsumeSymbol(p);

  return false;
}

static bool String(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("String");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  Val bin = PutString(&p->image->strings, p->token.lexeme + 1, p->token.length - 2);
  PutInst(p->module, OP_CONST, bin);
  AdvanceToken(p);

  return false;
}

static bool Number(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Number");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

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
      PutInst(p->module, OP_CONST, NumVal(f));
      return false;
    }

    u32 digit = token.lexeme[i] - '0';
    num = num*10 + digit;
  }

  if (num == 0) {
    PutInst(p->module, OP_ZERO);
  } else {
    PutInst(p->module, OP_CONST, IntVal(num));
  }

  return false;
}

static bool Literal(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Literal");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  switch (CurToken(p)) {
  case TOKEN_TRUE:
    PutInst(p->module, OP_TRUE);
    break;
  case TOKEN_FALSE:
    PutInst(p->module, OP_FALSE);
    break;
  case TOKEN_NIL:
    PutInst(p->module, OP_NIL);
    break;
  default:
    // error
    break;
  }
  AdvanceToken(p);

  return false;
}

static bool Access(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Access");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_DOT);
  ConsumeSymbol(p);
  PutInst(p->module, OP_ACCESS);

  return true;
}

static bool Operator(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Operator");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  TokenType op = CurToken(p);
  ParseRule *rule = GetRule(p);

  AdvanceToken(p);
  Subexpr(p, rule->prec + 1, false);

  switch (op) {
  case TOKEN_PLUS:
    PutInst(p->module, OP_ADD);
    break;
  case TOKEN_MINUS:
    PutInst(p->module, OP_SUB);
    break;
  case TOKEN_STAR:
    PutInst(p->module, OP_MUL);
    break;
  case TOKEN_SLASH:
    PutInst(p->module, OP_DIV);
    break;
  case TOKEN_BAR:
    PutInst(p->module, OP_PAIR);
    break;
  case TOKEN_EQ:
    PutInst(p->module, OP_EQUAL);
    break;
  case TOKEN_NEQ:
    PutInst(p->module, OP_EQUAL);
    PutInst(p->module, OP_NOT);
    break;
  case TOKEN_GT:
    PutInst(p->module, OP_GT);
    break;
  case TOKEN_GTE:
    PutInst(p->module, OP_LT);
    PutInst(p->module, OP_NOT);
    break;
  case TOKEN_LT:
    PutInst(p->module, OP_LT);
    break;
  case TOKEN_LTE:
    PutInst(p->module, OP_GT);
    PutInst(p->module, OP_NOT);
    break;
  default:
    // error
    break;
  }

  return false;
}

static bool Logic(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Logic");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  if (CurToken(p) == TOKEN_AND) {
    PutInst(p->module, OP_NOT);
  }

  // duplicate stack top, so our branch doesn't consume it
  PutInst(p->module, OP_DUP);
  PutInst(p->module, OP_BRANCH, 0);
  u32 branch = ChunkSize(p->module);

  // if we get to the second clause, we can discard the result from the first
  PutInst(p->module, OP_POP);

  ParseRule *rule = GetRule(p);
  AdvanceToken(p);
  Subexpr(p, rule->prec + 1, tail);

  SetByte(p->module, branch - 1, ChunkSize(p->module) - branch);

  return true;
}

static bool Mod(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Module");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_MODULE);
  ConsumeSymbol(p);

  PutInst(p->module, OP_SCOPE);
  Do(p, false);
  PutInst(p->module, OP_POP);
  PutInst(p->module, OP_MODULE);

  return false;
}

static bool Import(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Import");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_IMPORT);
  ConsumeSymbol(p);
  PutInst(p->module, OP_IMPORT);
  return false;
}

static bool Use(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Use");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_USE);
  ConsumeSymbol(p);
  PutInst(p->module, OP_USE);
  return false;
}

static bool Let(Parser *p, bool tail)
{
#ifdef DEBUG_COMPILE
  Print("Let");
  if (tail) Print(" (tail)");
  Print("\n");
#endif

  ExpectToken(p, TOKEN_LET);
  ExpectToken(p, TOKEN_LBRACE);

  PutInst(p->module, OP_SCOPE);

  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    ConsumeSymbol(p);
    ExpectToken(p, TOKEN_COLON);
    Expr(p, false);
    num++;
    MatchToken(p, TOKEN_COMMA);
    PutInst(p->module, OP_DEFINE);
    PutInst(p->module, OP_POP);
  }

  bool callable = Do(p, false);
  PutInst(p->module, OP_UNSCOPE);
  return callable;
}

/* Helpers */

static Val PutSymbol(Module *module, StringMap *strings, char *name, u32 length)
{
  return MakeSymbolFromSlice(strings, name, length);
}

static void ConsumeSymbol(Parser *p)
{
  Val sym = PutSymbol(p->module, &p->image->strings, p->token.lexeme, p->token.length);
  PutInst(p->module, OP_CONST, sym);
  AdvanceToken(p);
}

static char *PrecStr(Precedence prec)
{
  switch (prec)
  {
  case PREC_NONE:     return "NONE";
  // case PREC_BLOCK:    return "BLOCK";
  // case PREC_PIPE:     return "PIPE";
  case PREC_EXPR:     return "EXPR";
  case PREC_LOGIC:    return "LOGIC";
  case PREC_EQUALITY: return "EQUALITY";
  case PREC_COMPARE:  return "COMPARE";
  case PREC_PAIR:     return "PAIR";
  case PREC_TERM:     return "TERM";
  case PREC_FACTOR:   return "FACTOR";
  // case PREC_EXPONENT: return "EXPONENT";
  // case PREC_LAMBDA:   return "LAMBDA";
  case PREC_UNARY:    return "UNARY";
  case PREC_ACCESS:   return "ACCESS";
  // case PREC_PRIMARY:  return "PRIMARY";
  }
}
