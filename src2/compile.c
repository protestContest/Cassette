#include "compile.h"
#include "lex.h"
#include "vm.h"

typedef struct {
  Token token;
  char **source;
  Chunk *chunk;
  u32 line;
  u32 col;
  bool error;
} Compiler;

typedef void (*ParseFn)(Compiler *compiler);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  u32 precedence;
} ParseRule;

void AdvanceToken(Compiler *compiler);
void CompileNum(Compiler *compiler);
void CompileString(Compiler *compiler);
void CompileLiteral(Compiler *compiler);
void CompileSymbol(Compiler *compiler);
void CompileVariable(Compiler *compiler);
void CompileGroup(Compiler *compiler);
void CompileUnary(Compiler *compiler);
void CompileInfixLeft(Compiler *compiler);
void CompileInfixRight(Compiler *compiler);
void CompileLogic(Compiler *compiler);
void CompileDoBlock(Compiler *compiler);
void CompileIfBlock(Compiler *compiler);
void CompileCondBlock(Compiler *compiler);
void CompileLet(Compiler *compiler);
void CompileDef(Compiler *compiler);
void CompileAccess(Compiler *compiler);
void CompileList(Compiler *compiler);
void CompileTuple(Compiler *compiler);
void CompileMap(Compiler *compiler);

static ParseRule rules[] = {
  [TokenEOF]          = {NULL, NULL, 0},
  [TokenError]        = {NULL, NULL, 0},
  [TokenLet]          = {&CompileLet, NULL, 0},
  [TokenComma]        = {NULL, NULL, 0},
  [TokenEqual]        = {NULL, NULL, 0},
  [TokenDef]          = {&CompileDef, NULL, 0},
  [TokenLParen]       = {&CompileGroup, NULL, 0},
  [TokenRParen]       = {NULL, NULL, 0},
  [TokenID]           = {&CompileVariable, NULL, 0},
  [TokenArrow]        = {NULL, &CompileInfixLeft, 2},
  [TokenAnd]          = {NULL, &CompileLogic, 3},
  [TokenOr]           = {NULL, &CompileLogic, 3},
  [TokenEqualEqual]   = {NULL, &CompileInfixLeft, 4},
  [TokenNotEqual]     = {NULL, &CompileInfixLeft, 4},
  [TokenGreater]      = {NULL, &CompileInfixLeft, 5},
  [TokenGreaterEqual] = {NULL, &CompileInfixLeft, 5},
  [TokenLess]         = {NULL, &CompileInfixLeft, 5},
  [TokenLessEqual]    = {NULL, &CompileInfixLeft, 5},
  [TokenIn]           = {NULL, &CompileInfixLeft, 6},
  [TokenPlus]         = {NULL, &CompileInfixRight, 7},
  [TokenMinus]        = {&CompileUnary, &CompileInfixRight, 7},
  [TokenStar]         = {NULL, &CompileInfixRight, 8},
  [TokenSlash]        = {NULL, &CompileInfixRight, 8},
  [TokenNot]          = {&CompileUnary, NULL, 0},
  [TokenNum]          = {&CompileNum, NULL, 0},
  [TokenString]       = {&CompileString, NULL, 0},
  [TokenTrue]         = {&CompileLiteral, NULL, 0},
  [TokenFalse]        = {&CompileLiteral, NULL, 0},
  [TokenNil]          = {&CompileLiteral, NULL, 0},
  [TokenColon]        = {&CompileSymbol, NULL, 0},
  [TokenDot]          = {&CompileAccess, NULL, 0},
  [TokenDo]           = {&CompileDoBlock, NULL, 0},
  [TokenEnd]          = {NULL, NULL, 0},
  [TokenIf]           = {&CompileIfBlock, NULL, 0},
  [TokenElse]         = {NULL, NULL, 0},
  [TokenCond]         = {&CompileCondBlock, NULL, 0},
  [TokenLBracket]     = {&CompileList, NULL, 0},
  [TokenRBracket]     = {NULL, NULL, 0},
  [TokenHashBracket]  = {&CompileTuple, NULL, 0},
  [TokenLBrace]       = {NULL, NULL, 0},
  [TokenRBrace]       = {&CompileMap, NULL, 0},
  [TokenNewline]      = {&AdvanceToken, NULL, 0},
};

void InitCompiler(Compiler *compiler, Chunk *chunk, char **source)
{
  compiler->chunk = chunk;
  compiler->source = source;
  compiler->line = 1;
  compiler->col = 1;
  compiler->error = false;
  AdvanceToken(compiler);
}

bool Expect(bool assertion, char *message)
{
  if (!assertion) {
    Print(message);
    Print("\n");
  }
  return assertion;
}

void AdvanceToken(Compiler *compiler)
{
  if (compiler->token.type == TokenNewline) {
    compiler->line++;
    compiler->col = 1;
  }
  char *start = *compiler->source;
  compiler->token = NextToken(compiler->source);
  compiler->col += *compiler->source - start;
}

void SkipNewlines(Compiler *compiler)
{
  while (compiler->token.type == TokenNewline) AdvanceToken(compiler);
}

void CompileError(Compiler *compiler, char *message)
{
  Print("Compile error [");
  PrintInt(compiler->line);
  Print(":");
  PrintInt(compiler->col);
  Print("]: ");
  Print(message);
  Print("\n");
  compiler->error = true;
}

void CompileExpr(Compiler *compiler, u32 precedence)
{
  if (compiler->error) return;

  Print("Compiling ");
  PrintToken(compiler->token);
  Print("\n");

  ParseRule rule = rules[compiler->token.type];
  if (!Expect(rule.prefix != NULL, "Expected expression")) return;

  rule.prefix(compiler);

  rule = rules[compiler->token.type];
  while (rule.precedence >= precedence) {
    if (!Expect(rule.infix, "No infix")) return;

    Print("Compiling ");
    PrintToken(compiler->token);
    Print("\n");

    rule.infix(compiler);
    rule = rules[compiler->token.type];
  }
}

void CompileStatement(Compiler *compiler)
{
  if (compiler->error) return;
  SkipNewlines(compiler);

  Print("Compiling statement\n");

  u32 num_exprs = 0;
  while (compiler->token.type != TokenEOF && compiler->token.type != TokenNewline) {
    num_exprs++;
    CompileExpr(compiler, 1);
  }

  if (num_exprs > 1) {
    PushByte(compiler->chunk, OpApply);
    PushByte(compiler->chunk, num_exprs);
  }

  SkipNewlines(compiler);
}

Chunk Compile(char *source)
{
  Chunk chunk;
  InitChunk(&chunk);
  Compiler compiler;
  InitCompiler(&compiler, &chunk, &source);

  while (compiler.token.type != TokenEOF) {
    CompileStatement(&compiler);
  }

  return *compiler.chunk;
}

void CompileID(Compiler *compiler, Token id)
{
  Val sym = MakeSymbol(id.lexeme, id.length, &compiler->chunk->constants);
  PushByte(compiler->chunk, OpConst);
  PushByte(compiler->chunk, PushConst(compiler->chunk, sym));
}

void CompileNum(Compiler *compiler)
{
  PushByte(compiler->chunk, OpConst);
  PushByte(compiler->chunk, PushConst(compiler->chunk, compiler->token.value));
  AdvanceToken(compiler);
}

void CompileString(Compiler *compiler)
{
  CompileID(compiler, compiler->token);
  PushByte(compiler->chunk, OpStr);
  AdvanceToken(compiler);
}

void CompileLiteral(Compiler *compiler)
{
  switch (compiler->token.type) {
  case TokenTrue:
    PushByte(compiler->chunk, OpTrue);
    break;
  case TokenFalse:
    PushByte(compiler->chunk, OpFalse);
    break;
  case TokenNil:
    PushByte(compiler->chunk, OpNil);
    break;
  default: break;
  }
  AdvanceToken(compiler);
}

void CompileSymbol(Compiler *compiler)
{
  AdvanceToken(compiler);
  CompileID(compiler, compiler->token);
  AdvanceToken(compiler);
}

void CompileLambda(Compiler *compiler, Token *ids)
{
  // jump past lambda body
  PushByte(compiler->chunk, OpJump);
  u32 patch = VecCount(compiler->chunk->data);
  PushByte(compiler->chunk, 0); // patched later

  // lambda body
  // args are on the stack; define each param
  for (u32 i = VecCount(ids); i > 0; i--) {
    Val sym = MakeSymbol(ids[i-1].lexeme, ids[i-1].length, &compiler->chunk->constants);
    PushByte(compiler->chunk, OpConst);
    PushByte(compiler->chunk, PushConst(compiler->chunk, sym));
    PushByte(compiler->chunk, OpDefine);
  }
  PushByte(compiler->chunk, OpPop); // pop the lambda from the stack

  CompileExpr(compiler, 1);
  PushByte(compiler->chunk, OpReturn);

  // after body
  // patch jump, create lambda
  compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
  PushByte(compiler->chunk, OpLambda);
  PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(patch+1)));
}

void CompileVariable(Compiler *compiler)
{
  CompileID(compiler, compiler->token);
  PushByte(compiler->chunk, OpLookup);
  AdvanceToken(compiler);
}

void CompileGroup(Compiler *compiler)
{
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  // try to store up IDs in case it's a lambda
  Token *ids = NULL;
  while (compiler->token.type == TokenID) {
    VecPush(ids, compiler->token);
    AdvanceToken(compiler);
    SkipNewlines(compiler);
  }

  if (compiler->token.type == TokenRParen) {
    AdvanceToken(compiler);
    if (compiler->token.type == TokenArrow) {
      AdvanceToken(compiler);
      SkipNewlines(compiler);
      CompileLambda(compiler, ids);
      FreeVec(ids);
      return;
    }

    // just a list of ids
    for (u32 i = 0; i < VecCount(ids); i++) {
      CompileID(compiler, ids[i]);
      PushByte(compiler->chunk, OpLookup);
    }
    PushByte(compiler->chunk, OpApply);
    PushByte(compiler->chunk, VecCount(ids));
    FreeVec(ids);
    return;
  }

  // not a list of ids, just an expression
  for (u32 i = 0; i < VecCount(ids); i++) {
    CompileID(compiler, ids[i]);
    PushByte(compiler->chunk, OpLookup);
  }
  u8 num_params = VecCount(ids);
  FreeVec(ids);

  while (compiler->token.type != TokenRParen) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    num_params++;
    CompileExpr(compiler, 1);
  }

  PushByte(compiler->chunk, OpApply);
  PushByte(compiler->chunk, num_params);
  AdvanceToken(compiler);
}

void CompileUnary(Compiler *compiler)
{
  TokenType op = compiler->token.type;

  u32 prec = rules[compiler->token.type].precedence;
  AdvanceToken(compiler);
  CompileExpr(compiler, prec);

  switch (op) {
  case TokenNot:
    PushByte(compiler->chunk, OpNot);
    break;
  case TokenMinus:
    PushByte(compiler->chunk, OpNeg);
    break;
  default: break;
  }
}

void CompileInfixLeft(Compiler *compiler)
{
  TokenType op = compiler->token.type;
  u32 prec = rules[compiler->token.type].precedence + 1;
  AdvanceToken(compiler);
  SkipNewlines(compiler);
  CompileExpr(compiler, prec);

  switch (op) {
  case TokenArrow:
    PushByte(compiler->chunk, OpLambda);
    break;
  case TokenEqualEqual:
    PushByte(compiler->chunk, OpEq);
    break;
  case TokenNotEqual:
    PushByte(compiler->chunk, OpEq);
    PushByte(compiler->chunk, OpNot);
    break;
  case TokenGreater:
    PushByte(compiler->chunk, OpGt);
    break;
  case TokenGreaterEqual:
    PushByte(compiler->chunk, OpLt);
    PushByte(compiler->chunk, OpNot);
    break;
  case TokenLess:
    PushByte(compiler->chunk, OpLt);
    break;
  case TokenLessEqual:
    PushByte(compiler->chunk, OpGt);
    PushByte(compiler->chunk, OpNot);
    break;
  case TokenIn:
    PushByte(compiler->chunk, OpIn);
    break;
  default: break;
  }
}

void CompileInfixRight(Compiler *compiler)
{
  TokenType op = compiler->token.type;
  u32 prec = rules[compiler->token.type].precedence;
  AdvanceToken(compiler);
  SkipNewlines(compiler);
  CompileExpr(compiler, prec);

  switch (op) {
  case TokenPlus:
    PushByte(compiler->chunk, OpAdd);
    break;
  case TokenMinus:
    PushByte(compiler->chunk, OpSub);
    break;
  case TokenStar:
    PushByte(compiler->chunk, OpMul);
    break;
  case TokenSlash:
    PushByte(compiler->chunk, OpDiv);
    break;
  default: break;
  }
}

void CompileLogic(Compiler *compiler)
{
  TokenType op = compiler->token.type;
  u32 prec = rules[compiler->token.type].precedence;
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  if (op == TokenAnd) {
    PushByte(compiler->chunk, OpBranchF);
  } else {
    PushByte(compiler->chunk, OpBranch);
  }

  u32 patch = VecCount(compiler->chunk->data);
  PushByte(compiler->chunk, 0);

  PushByte(compiler->chunk, OpPop);
  CompileExpr(compiler, prec);

  compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
}

void CompileDoBlock(Compiler *compiler)
{
  AdvanceToken(compiler);
  while (compiler->token.type != TokenEnd) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    CompileStatement(compiler);
  }
  AdvanceToken(compiler);
}

void CompileIfBlock(Compiler *compiler)
{
  AdvanceToken(compiler);
  CompileExpr(compiler, 1);

  PushByte(compiler->chunk, OpBranchF);
  u32 branch_false = VecCount(compiler->chunk->data);
  PushByte(compiler->chunk, 0);

  if (compiler->token.type != TokenDo) {
    CompileError(compiler, "Expected \"do\" after condition");
    return;
  }
  AdvanceToken(compiler);

  // true branch
  PushByte(compiler->chunk, OpPop); // discard condition
  while (compiler->token.type != TokenEnd && compiler->token.type != TokenElse) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    CompileStatement(compiler);
    PushByte(compiler->chunk, OpPop);
  }
  RewindVec(compiler->chunk->data, 1);

  // jump past false branch
  PushByte(compiler->chunk, OpJump);
  u32 jump_after = VecCount(compiler->chunk->data);
  PushByte(compiler->chunk, 0);

  // false branch
  compiler->chunk->data[branch_false] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
  PushByte(compiler->chunk, OpPop); // discard condition
  if (compiler->token.type == TokenElse) {
    AdvanceToken(compiler);
    while (compiler->token.type != TokenEnd) {
      if (compiler->token.type == TokenEOF) {
        CompileError(compiler, "Unexpected end of file");
        return;
      }
      CompileStatement(compiler);
      PushByte(compiler->chunk, OpPop);
    }
    RewindVec(compiler->chunk->data, 1);
  } else {
    // default else
    PushByte(compiler->chunk, OpNil);
  }

  compiler->chunk->data[jump_after] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
  AdvanceToken(compiler);
}

void CompileCondBlock(Compiler *compiler)
{
  AdvanceToken(compiler);
  if (compiler->token.type != TokenDo) {
    CompileError(compiler, "Expected \"do\" after \"cond\"");
    return;
  }
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  u32 *patches = NULL;
  while (compiler->token.type != TokenEnd) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }

    // condition
    CompileExpr(compiler, 3); // don't allow lambdas in condition
    SkipNewlines(compiler);
    if (compiler->token.type != TokenArrow) {
      CompileError(compiler, "Expected \"->\" after clause condition");
      return;
    }
    AdvanceToken(compiler);
    SkipNewlines(compiler);

    // skip consequent when false
    PushByte(compiler->chunk, OpBranchF);
    u32 patch = VecCount(compiler->chunk->data);
    PushByte(compiler->chunk, 0);

    // consequent
    CompileStatement(compiler);

    // jump to after cond
    PushByte(compiler->chunk, OpJump);
    VecPush(patches, VecCount(compiler->chunk->data));
    PushByte(compiler->chunk, 0);

    compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
  }

  u8 after_cond = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
  for (u32 i = 0; i < VecCount(patches); i++) {
    compiler->chunk->data[patches[i]] = after_cond;
  }
  FreeVec(patches);

  AdvanceToken(compiler);
}

void CompileAssign(Compiler *compiler)
{
  if (compiler->token.type != TokenID) {
    CompileError(compiler, "Expected identifier");
    return;
  }

  Token id = compiler->token;
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  if (compiler->token.type != TokenEqual) {
    CompileError(compiler, "Expected \"=\" after identifier");
    return;
  }
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  CompileExpr(compiler, 1);
  CompileID(compiler, id);
  PushByte(compiler->chunk, OpDefine);
}

void CompileLet(Compiler *compiler)
{
  AdvanceToken(compiler);
  SkipNewlines(compiler);

  CompileAssign(compiler);
  while (compiler->token.type == TokenComma) {
    AdvanceToken(compiler);
    SkipNewlines(compiler);
    CompileAssign(compiler);
  }
}

void CompileDef(Compiler *compiler)
{
  AdvanceToken(compiler);
  if (compiler->token.type == TokenLParen) {
    AdvanceToken(compiler);
    if (compiler->token.type != TokenID) {
      CompileError(compiler, "Expected identifier");
      return;
    }
    Token id = compiler->token;

    Token *params = NULL;
    while (compiler->token.type == TokenID) {
      VecPush(params, compiler->token);
      AdvanceToken(compiler);
      SkipNewlines(compiler);
    }
    if (compiler->token.type != TokenRParen) {
      CompileError(compiler, "Expected \")\"");
      return;
    }
    AdvanceToken(compiler);

    CompileLambda(compiler, params);
    CompileID(compiler, id);
    PushByte(compiler->chunk, OpDefine);
  } else if (compiler->token.type == TokenID) {
    Token id = compiler->token;
    AdvanceToken(compiler);
    CompileExpr(compiler, 1);
    CompileID(compiler, id);
    PushByte(compiler->chunk, OpDefine);
  }
}

void CompileAccess(Compiler *compiler)
{
  AdvanceToken(compiler);
  if (compiler->token.type != TokenID) {
    CompileError(compiler, "Expected identifier");
    return;
  }
  CompileID(compiler, compiler->token);
  AdvanceToken(compiler);
  PushByte(compiler->chunk, OpAccess);
}

void CompileList(Compiler *compiler)
{
  AdvanceToken(compiler);
  SkipNewlines(compiler);
  u32 num_items = 0;
  while (compiler->token.type != TokenRBracket) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    CompileExpr(compiler, 1);
    num_items++;
    if (compiler->token.type == TokenComma) AdvanceToken(compiler);
    SkipNewlines(compiler);
  }
  AdvanceToken(compiler);
  PushByte(compiler->chunk, OpList);
  PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
}

void CompileTuple(Compiler *compiler)
{
  AdvanceToken(compiler);
  SkipNewlines(compiler);
  u32 num_items = 0;
  while (compiler->token.type != TokenRBracket) {
    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    CompileExpr(compiler, 1);
    num_items++;
    if (compiler->token.type == TokenComma) AdvanceToken(compiler);
    SkipNewlines(compiler);
  }
  AdvanceToken(compiler);
  PushByte(compiler->chunk, OpTuple);
  PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
}

void CompileMap(Compiler *compiler)
{
  AdvanceToken(compiler);
  SkipNewlines(compiler);
  u32 num_items = 0;
  while (compiler->token.type != TokenRBrace) {
    if (compiler->token.type != TokenID) {
      CompileError(compiler, "Expected identifier");
      return;
    }
    Token id = compiler->token;
    AdvanceToken(compiler);
    if (compiler->token.type != TokenColon) {
      CompileError(compiler, "Expected \":\"");
      return;
    }
    AdvanceToken(compiler);
    SkipNewlines(compiler);

    if (compiler->token.type == TokenEOF) {
      CompileError(compiler, "Unexpected end of file");
      return;
    }
    CompileExpr(compiler, 1);
    CompileID(compiler, id);
    num_items++;

    if (compiler->token.type == TokenComma) AdvanceToken(compiler);
    SkipNewlines(compiler);
  }
  AdvanceToken(compiler);
  PushByte(compiler->chunk, OpMap);
  PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
}
