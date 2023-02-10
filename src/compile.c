#include "compile.h"
#include "chunk.h"
#include "mem.h"
#include "ops.h"
#include "scan.h"
#include "vec.h"
#include "vm.h"
#include "printer.h"

#define DEBUG_COMPILE 1

typedef enum Precedence {
  PREC_NONE,
  // PREC_BLOCK,
  // PREC_PIPE,
  PREC_EXPR,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_PAIR,
  PREC_TERM,
  PREC_FACTOR,
  // PREC_EXPONENT,
  // PREC_LAMBDA,
  PREC_UNARY,
  PREC_ACCESS,
  // PREC_PRIMARY,
} Precedence;

typedef void(*ParseFn)(Parser *p);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
} ParseRule;

static void Script(Parser *p);
static u32 Call(Parser *p, bool skip_newlines);
static ParseFn Expression(Parser *p, Precedence prec, bool skip_newlines);

static void Module(Parser *p);

// helpers
static bool IsCallable(ParseFn parsed);
static void ConsumeSymbol(Parser *p);
static char *PrecStr(Precedence prec);

/* Compiles the given source as a script into a chunk.
 */
Status Compile(char *src, Chunk *chunk)
{
  Parser p;
  InitParser(&p, src, chunk);

  // Prime the parser
  AdvanceToken(&p);
  Script(&p);

  if (DEBUG_COMPILE) Disassemble("Script", chunk);

  return Ok;
}

/* Same as `Compile`, but only compiles module expressions.
 */
Status CompileModule(char *src, Chunk *chunk)
{
  Parser p;
  InitParser(&p, src, chunk);

  AdvanceToken(&p);
  while (CurToken(&p) != TOKEN_EOF) {
    while (CurToken(&p) != TOKEN_MODULE && CurToken(&p) != TOKEN_EOF) AdvanceToken(&p);
    if (CurToken(&p) == TOKEN_EOF) break;
    Module(&p);
  }

  if (DEBUG_COMPILE) Disassemble("Module", chunk);

  return Ok;
}

/* A script is parsed as a sequence of function calls. The result of each call
 * is discarded, except the last. Calls are separated by newlines or commas.
 */
static void Script(Parser *p)
{
  if (DEBUG_COMPILE) printf("Script\n");

  while (!MatchToken(p, TOKEN_EOF)) {
    Call(p, false);
    SkipNewlines(p);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }
  // don't pop the final value
  RewindVec(p->chunk->code, 1);
}

/* A call is a sequence of expressions, separated by spaces. The first
 * expression is the operator, followed by arguments. If there are no arguments,
 * the expression isn't called. Returns the number of arguments.
 */
static u32 Call(Parser *p, bool skip_newlines)
{
  if (DEBUG_COMPILE) printf("Expression\n");

  u32 start = ChunkSize(p->chunk);

  // first expression is possibly the operator
  Expression(p, PREC_EXPR + 1, skip_newlines);

  // save the operator code for later
  u32 length = ChunkSize(p->chunk) - start;
  u8 operator_code[length];
  memcpy(operator_code, &p->chunk->code[start], length);
  RewindVec(p->chunk->code, length);

  // parse the arguments
  u32 args = 0;
  while (Expression(p, PREC_EXPR + 1, skip_newlines) != NULL) {
    args++;
  }

  // copy the operator code back to the end
  start = ChunkSize(p->chunk);
  GrowVec(p->chunk->code, length);
  memcpy(&p->chunk->code[start], operator_code, length);

  if (args > 0) {
    PutInst(p->chunk, OP_CALL);
  }

  return args;
}

/* Parse functions */
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
static void Import(Parser *p);
static void Let(Parser *p);

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
  [TOKEN_MODULE] =      { Module,           NULL,           PREC_NONE     },
  [TOKEN_IMPORT] =      { Import,           NULL,           PREC_NONE     },
  [TOKEN_LET] =         { Let,              NULL,           PREC_NONE     },
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

/* Parses an expression: a token that can be parsed as a prefix, followed by
 * tokens that can be parsed as infix operators, up to the given precedence.
 * Returns the last parse function executed.
 */
static ParseFn Expression(Parser *p, Precedence prec, bool skip_newlines)
{
  if (skip_newlines) SkipNewlines(p);

#if DEBUG_COMPILE
    printf("%-10s ", PrecStr(prec));
    printf("Prefix %s \"%.*s\"\n", TokenStr(CurToken(p)), p->token.length, p->token.lexeme);
    PrintSourceContext(p, 0);
#endif

  ParseRule *rule = GetRule(p);
  if (rule->prefix == NULL) {
    return NULL;
  }

  ParseFn parsed = rule->prefix;
  rule->prefix(p);
  if (skip_newlines) SkipNewlines(p);

  rule = GetRule(p);
  while (rule->prec >= prec) {
#if DEBUG_COMPILE
    printf("%-10s ", PrecStr(prec));
    printf("Infix %s \"%.*s\"\n", TokenStr(CurToken(p)), p->token.length, p->token.lexeme);
#endif

    parsed = rule->infix;
    rule->infix(p);
    if (skip_newlines) SkipNewlines(p);

    rule = GetRule(p);
  }

  return parsed;
}

/* Parses an expression in parentheses. Parenthesized expressions override
 * precedence, can span multiple lines, and are always called. Since lambdas are
 * defined with parenthesized parameters, this also checks for an arrow token
 * afterwards. If it finds one, it discards the code it generated and calls the
 * lambda parse function instead.
 */
static void Grouping(Parser *p)
{
  if (DEBUG_COMPILE) printf("Grouping\n");

  Parser saved = *p;
  u32 start = ChunkSize(p->chunk);

  ExpectToken(p, TOKEN_LPAREN);

  // Compile a multi-line expression
  if (Call(p, true) == 0) {
    // call the expression if it wasn't already
    PutInst(p->chunk, OP_CALL);
  }

  ExpectToken(p, TOKEN_RPAREN);

  if (CurToken(p) == TOKEN_ARROW) {
    *p = saved;
    RewindVec(p->chunk->code, ChunkSize(p->chunk) - start);
    Lambda(p);
  }
}

/* Parses a lambda definition. Puts the parameter symbols on the stack, defines
 * the body (and jumps past it), puts the body's location on the stack, and
 * creates a closure.
 */
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

  // jump past the body; will be patched once the body length is known
  PutInst(p->chunk, OP_JUMP, 0);
  u32 start = ChunkSize(p->chunk);

  // compile body
  Call(p, false);

  // if the last op was a function call, make it a tail-recursive apply instead
  if (VecLast(p->chunk->code) == OP_CALL) {
    SetByte(p->chunk, ChunkSize(p->chunk) - 1, OP_APPLY);
  }

  // always add a return op, in case branches in the body need to jump here
  PutInst(p->chunk, OP_RETURN);

  // patch jump op to skip past the body
  SetByte(p->chunk, start - 1, ChunkSize(p->chunk) - start);

  // define lambda (params, position on stack)
  PutInst(p->chunk, OP_CONST, IntVal(start));
  PutInst(p->chunk, OP_LAMBDA, num_params);
}

/* Parses a definition. Puts a symbol on the stack, followed by the result of an
 * expression, and defines them in the current environment.
 */
static void Define(Parser *p)
{
  if (DEBUG_COMPILE) printf("Define\n");

  ExpectToken(p, TOKEN_DEF);

  if (MatchToken(p, TOKEN_LPAREN)) {
    ConsumeSymbol(p);

    // params
    u32 num_params = 0;
    while (!MatchToken(p, TOKEN_RPAREN)) {
      num_params++;
      ConsumeSymbol(p);
      MatchToken(p, TOKEN_COMMA);
    }

    // jump past the body; will be patched once the body length is known
    PutInst(p->chunk, OP_JUMP, 0);
    u32 start = ChunkSize(p->chunk);

    // compile body
    // Expression(p, PREC_EXPR + 1, false);
    Call(p, false);

    // if the last op was a function call, make it a tail-recursive apply instead
    if (VecLast(p->chunk->code) == OP_CALL) {
      SetByte(p->chunk, ChunkSize(p->chunk) - 1, OP_APPLY);
    }

    // always add a return op, in case branches in the body need to jump here
    PutInst(p->chunk, OP_RETURN);

    // patch jump op to skip past the body
    SetByte(p->chunk, start - 1, ChunkSize(p->chunk) - start);

    // define lambda (params, position on stack)
    PutInst(p->chunk, OP_CONST, IntVal(start));
    PutInst(p->chunk, OP_LAMBDA, num_params);
  } else {
    ConsumeSymbol(p);
    Call(p, false);
  }

  PutInst(p->chunk, OP_DEFINE);
}

/* Parses a sequence of expressions separated by newlines. The result of each
 * expression is discarded, except the last.
 */
static void Do(Parser *p)
{
  if (DEBUG_COMPILE) printf("Do\n");

  ExpectToken(p, TOKEN_DO);
  SkipNewlines(p);

  while (!MatchToken(p, TOKEN_END)) {
    Call(p, false);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }

  // don't pop the final value
  RewindVec(p->chunk->code, 1);
}

/* Same as `Do`, except this stops at an "else" or "end" keyword without
 * consuming it.
 */
static void DoElse(Parser *p)
{
  ExpectToken(p, TOKEN_DO);
  SkipNewlines(p);

  while (CurToken(p) != TOKEN_END && CurToken(p) != TOKEN_ELSE) {
    Call(p, false);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }
  // don't pop the final value
  RewindVec(p->chunk->code, 1);
}

/* Same as `Do`, except beginning with an "else" keyword
 */
static void Else(Parser *p)
{
  ExpectToken(p, TOKEN_ELSE);
  SkipNewlines(p);

  while (!MatchToken(p, TOKEN_END)) {
    Call(p, false);
    MatchToken(p, TOKEN_COMMA);
    SkipNewlines(p);
    PutInst(p->chunk, OP_POP);
  }
  // don't pop the final value
  RewindVec(p->chunk->code, 1);
}

/* Compiles an if expression: the first expression is the condition, and the
 * second is the consequent. If the consequent is a "do" expression, it's parsed
 * as a possible "do-else" block.
 */
static void If(Parser *p)
{
  if (DEBUG_COMPILE) printf("If\n");

  ExpectToken(p, TOKEN_IF);

  // condition
  Expression(p, PREC_EXPR + 1, false);

  // jump past the consequent if it's false
  PutInst(p->chunk, OP_NOT);
  PutInst(p->chunk, OP_BRANCH, 0);
  u32 consequent = ChunkSize(p->chunk);

  // check for do-else
  bool do_else = (CurToken(p) == TOKEN_DO);

  // consequent
  if (do_else) {
    DoElse(p);
  } else {
    Expression(p, PREC_EXPR + 1, false);
  }

  // jump past the alternative
  PutInst(p->chunk, OP_JUMP, 0);

  // patch jump over consequent to here
  u32 alternative = ChunkSize(p->chunk);
  SetByte(p->chunk, consequent - 1, alternative - consequent);

  // alternative (nil unless using a do/else block)
  if (do_else && CurToken(p) == TOKEN_ELSE) {
    Else(p);
  } else {
    PutInst(p->chunk, OP_NIL);
  }

  // patch jump over alternative to here
  u32 after = ChunkSize(p->chunk);
  SetByte(p->chunk, alternative - 1, after - alternative);
}

/* Parses a cond expression: a sequence of clauses. A clause is a condition
 * expression, followed by an arrow, followed by a result expression.
 */
static void Cond(Parser *p)
{
  // TODO
}

/* Parses a unary operator: "-" or "not", followed by an expression
 */
static void Unary(Parser *p)
{
  if (DEBUG_COMPILE) printf("Unary\n");

  TokenType op = CurToken(p);
  AdvanceToken(p);
  ExpectToken(p, TOKEN_MINUS);
  Expression(p, PREC_UNARY, false);

  if (op == TOKEN_MINUS) {
    PutInst(p->chunk, OP_NEG);
  } else if (op == TOKEN_NOT) {
    PutInst(p->chunk, OP_NOT);
  } else {
    Fatal("Invalid unary op: %s", TokenStr(op));
  }
}

/* Parses a literal list: a sequence of expressions separated by commas
 */
static void List(Parser *p)
{
  if (DEBUG_COMPILE) printf("List\n");

  ExpectToken(p, TOKEN_LBRACKET);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACKET)) {
    Call(p, true);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->chunk, OP_LIST, num);
}

/* Parses a literal dict: a sequence of key-value pairs separated by commas.
 * Each key-value pair is a symbol, a colon, and an expression.
 */
static void Dict(Parser *p)
{
  if (DEBUG_COMPILE) printf("Dict\n");

  ExpectToken(p, TOKEN_LBRACE);
  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    ConsumeSymbol(p);
    ExpectToken(p, TOKEN_COLON);
    Call(p, true);
    num++;
    MatchToken(p, TOKEN_COMMA);
  }
  PutInst(p->chunk, OP_DICT, num);
}

/* Parses a variable: a symbol to be looked up in the environment. */
static void Variable(Parser *p)
{
  if (DEBUG_COMPILE) printf("Variable\n");

  ConsumeSymbol(p);
  PutInst(p->chunk, OP_LOOKUP);
}

/* Parses a quoted symbol, where the symbol is pushed onto the stack without
 * being looked up.
 */
static void Sym(Parser *p)
{
  if (DEBUG_COMPILE) printf("Symbol\n");

  ExpectToken(p, TOKEN_COLON);
  ConsumeSymbol(p);
}

/* Parses a string: any sequence of characters (except double-quote)
 */
static void String(Parser *p)
{
  if (DEBUG_COMPILE) printf("String\n");

  Val bin = PutString(&p->chunk->strings, p->token.lexeme + 1, p->token.length - 2);
  PutInst(p->chunk, OP_CONST, bin);
  AdvanceToken(p);
}

/* Parses a number. Underscores are ignored.
 */
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

/* Parses a literal value: true, false, or nil
 */
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

/* Parses a dict access. A dict should be on the stack.
 */
static void Access(Parser *p)
{
  if (DEBUG_COMPILE) printf("Access\n");

  ExpectToken(p, TOKEN_DOT);
  ConsumeSymbol(p);
  PutInst(p->chunk, OP_ACCESS);
}

/* Parses an infix operator. The first operand should already be compiled; this
 * compiles the second operand expression (according to the operator's
 * precedence), and adds the operator's instruction.
 */
static void Operator(Parser *p)
{
  if (DEBUG_COMPILE) printf("Operator\n");

  TokenType op = CurToken(p);
  ParseRule *rule = GetRule(p);

  AdvanceToken(p);
  Expression(p, rule->prec + 1, true);

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

/* Parses a logical infix operator ("and" and "or"). Unlike other infix
 * operators, these short-ciruit execution.
 */
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
  Expression(p, rule->prec + 1, true);

  SetByte(p->chunk, branch - 1, ChunkSize(p->chunk) - branch);
}

static void Module(Parser *p)
{
  if (DEBUG_COMPILE) printf("Module\n");

  ExpectToken(p, TOKEN_MODULE);
  ConsumeSymbol(p);

  PutInst(p->chunk, OP_SCOPE);
  Do(p);
  PutInst(p->chunk, OP_POP);
  PutInst(p->chunk, OP_MODULE);
}

static void Import(Parser *p)
{
  if (DEBUG_COMPILE) printf("Import\n");

  ExpectToken(p, TOKEN_IMPORT);
  ConsumeSymbol(p);
  PutInst(p->chunk, OP_IMPORT);
}

static void Let(Parser *p)
{
  if (DEBUG_COMPILE) printf("Let\n");

  ExpectToken(p, TOKEN_LET);
  ExpectToken(p, TOKEN_LBRACE);

  PutInst(p->chunk, OP_SCOPE);

  u32 num = 0;
  while (!MatchToken(p, TOKEN_RBRACE)) {
    ConsumeSymbol(p);
    ExpectToken(p, TOKEN_COLON);
    Call(p, true);
    num++;
    MatchToken(p, TOKEN_COMMA);
    PutInst(p->chunk, OP_DEFINE);
    PutInst(p->chunk, OP_POP);
  }

  Do(p);

  PutInst(p->chunk, OP_UNSCOPE);
}

/* Helpers */

static void ConsumeSymbol(Parser *p)
{
  Val sym = PutSymbol(p->chunk, p->token.lexeme, p->token.length);
  PutInst(p->chunk, OP_CONST, sym);
  AdvanceToken(p);
}

static bool IsCallable(ParseFn parsed)
{
  return parsed == Grouping || parsed == Access || parsed == Variable;
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
