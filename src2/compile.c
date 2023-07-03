#include "compile.h"
#include "lex.h"
#include "vm.h"
#include "ops.h"
#include "assemble.h"
#include "source.h"

typedef struct Compiler {
  Token token;
  char *source;
  u32 line;
  u32 col;
  u32 pos;
  u32 next_label;
  bool ok;
  Mem mem;
} Compiler;

typedef Seq (*ParseFn)(Compiler *compiler, Linkage linkage);

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLambda,
  PrecLogic,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecSum,
  PrecProduct,
  PrecUnary
} Precedence;

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static Seq CompileExpr(Compiler *c, Linkage linkage, Precedence prec);
static Seq CompileNum(Compiler *c, Linkage linkage);
static Seq CompileVar(Compiler *c, Linkage linkage);
static Seq CompileLiteral(Compiler *c, Linkage linkage);
static Seq CompileSymbol(Compiler *c, Linkage linkage);
static Seq CompileGroup(Compiler *c, Linkage linkage);

static ParseRule rules[] = {
  [TokenNum]          = {&CompileNum, NULL, PrecNone},
  [TokenID]           = {&CompileVar, NULL, PrecNone},
  [TokenTrue]         = {&CompileLiteral, NULL, PrecNone},
  [TokenFalse]        = {&CompileLiteral, NULL, PrecNone},
  [TokenNil]          = {&CompileLiteral, NULL, PrecNone},
  [TokenColon]        = {&CompileSymbol, NULL, PrecNone},
  [TokenLParen]       = {&CompileGroup, NULL, PrecNone},
  [TokenEOF]          = {NULL, NULL, PrecNone},
  [TokenError]        = {NULL, NULL, PrecNone},
  [TokenLet]          = {NULL, NULL, PrecNone},
  [TokenComma]        = {NULL, NULL, PrecNone},
  [TokenEqual]        = {NULL, NULL, PrecNone},
  [TokenDef]          = {NULL, NULL, PrecNone},
  [TokenRParen]       = {NULL, NULL, PrecNone},
  [TokenArrow]        = {NULL, NULL, PrecNone},
  [TokenAnd]          = {NULL, NULL, PrecNone},
  [TokenOr]           = {NULL, NULL, PrecNone},
  [TokenEqualEqual]   = {NULL, NULL, PrecNone},
  [TokenNotEqual]     = {NULL, NULL, PrecNone},
  [TokenGreater]      = {NULL, NULL, PrecNone},
  [TokenGreaterEqual] = {NULL, NULL, PrecNone},
  [TokenLess]         = {NULL, NULL, PrecNone},
  [TokenLessEqual]    = {NULL, NULL, PrecNone},
  [TokenIn]           = {NULL, NULL, PrecNone},
  [TokenPipe]         = {NULL, NULL, PrecNone},
  [TokenPlus]         = {NULL, NULL, PrecNone},
  [TokenMinus]        = {NULL, NULL, PrecNone},
  [TokenStar]         = {NULL, NULL, PrecNone},
  [TokenSlash]        = {NULL, NULL, PrecNone},
  [TokenNot]          = {NULL, NULL, PrecNone},
  [TokenString]       = {NULL, NULL, PrecNone},
  [TokenDot]          = {NULL, NULL, PrecNone},
  [TokenDo]           = {NULL, NULL, PrecNone},
  [TokenEnd]          = {NULL, NULL, PrecNone},
  [TokenIf]           = {NULL, NULL, PrecNone},
  [TokenElse]         = {NULL, NULL, PrecNone},
  [TokenCond]         = {NULL, NULL, PrecNone},
  [TokenLBracket]     = {NULL, NULL, PrecNone},
  [TokenRBracket]     = {NULL, NULL, PrecNone},
  [TokenHashBracket]  = {NULL, NULL, PrecNone},
  [TokenLBrace]       = {NULL, NULL, PrecNone},
  [TokenRBrace]       = {NULL, NULL, PrecNone},
  [TokenNewline]      = {NULL, NULL, PrecNone},
  [TokenModule]       = {NULL, NULL, PrecNone},
};

void CompileError(Compiler *compiler, char *message)
{
  PrintEscape(IOFGRed);
  Print("Compile error [");
  PrintInt(compiler->line);
  Print(":");
  PrintInt(compiler->col);
  Print("]: ");
  Print(message);
  Print("\n");

  u32 token_pos = compiler->token.lexeme - compiler->source;
  PrintSourceContext(compiler->source, compiler->line, 3, token_pos, compiler->token.length);

  PrintEscape(IOFGReset);
  Print("\n");
  compiler->ok = false;
}

void AdvanceToken(Compiler *c)
{
  if (c->token.type == TokenNewline) {
    c->line++;
    c->col = 1;
  }
  char *start = c->source + c->pos;
  char *end = start;
  c->token = NextToken(&end);
  c->pos += end - start;
  c->col += end - start;
}

bool MatchToken(Compiler *c, TokenType type)
{
  if (c->token.type == type) {
    AdvanceToken(c);
    return true;
  }
  return false;
}

bool ExpectToken(Compiler *c, TokenType type)
{
  if (!MatchToken(c, type)) {
    CompileError(c, "Expected token");
    return false;
  }

  return true;
}

void SkipNewlines(Compiler *compiler)
{
  while (compiler->token.type == TokenNewline) AdvanceToken(compiler);
}

void InitCompiler(Compiler *compiler, char *source)
{
  compiler->source = source;
  compiler->line = 1;
  compiler->col = 1;
  compiler->pos = 0;
  compiler->ok = true;
  InitMem(&compiler->mem);
  AdvanceToken(compiler);
}

Chunk Compile(char *source)
{
  Compiler compiler;
  InitCompiler(&compiler, source);

  Seq code = CompileExpr(&compiler, LinkNext, PrecExpr);

  return Assemble(code, &compiler.mem);
}

static Seq CompileExpr(Compiler *c, Linkage linkage, Precedence prec)
{
  ParseRule rule = rules[c->token.type];
  if (rule.prefix == NULL) {
    CompileError(c, "Expected expression");
    return EmptySeq();
  }
  return rule.prefix(c, linkage);
}

static Seq CompileNum(Compiler *c, Linkage linkage)
{
  Mem *mem = &c->mem;
  Val value = c->token.value;
  AdvanceToken(c);

  return
    EndWithLinkage(
      MakeSeq(0, 0,
        Pair(OpSymbol(OpConst),
        Pair(value, nil, mem), mem)), linkage, mem);
}

static Seq CompileVar(Compiler *c, Linkage linkage)
{
  Mem *mem = &c->mem;
  Val id = MakeSymbol(c->token.lexeme, c->token.length, mem);
  AdvanceToken(c);

  return
    EndWithLinkage(
      MakeSeq(0, RegEnv,
        Pair(OpSymbol(OpConst),
        Pair(id,
        Pair(OpSymbol(OpLookup), nil, mem), mem), mem)), linkage, mem);
}

static Seq CompileSymbol(Compiler *c, Linkage linkage)
{
  Mem *mem = &c->mem;
  ExpectToken(c, TokenColon);
  Val id = MakeSymbol(c->token.lexeme, c->token.length, mem);
  AdvanceToken(c);

  return
    EndWithLinkage(
      MakeSeq(0, RegEnv,
        Pair(OpSymbol(OpConst),
        Pair(id, nil, mem), mem)), linkage, mem);
}

static Seq CompileLiteral(Compiler *c, Linkage linkage)
{
  OpCode op;
  switch (c->token.type) {
  case TokenTrue:
    op = OpTrue;
    break;
  case TokenFalse:
    op = OpFalse;
    break;
  case TokenNil:
    op = OpNil;
    break;
  default:
    CompileError(c, "Unknown literal");
    op = OpHalt;
  }
  AdvanceToken(c);

  return
    EndWithLinkage(
      MakeSeq(0, 0, Pair(OpSymbol(op), nil, &c->mem)), linkage, &c->mem);
}

static Seq CompileLambda(Compiler *c, Linkage linkage, Token *ids)
{
  Mem *mem = &c->mem;
  Val entry = MakeLabel(c->next_label++, mem);
  Val after_body = MakeLabel(c->next_label++, mem);

  Linkage lambda_linkage = (Eq(linkage, LinkNext)) ? after_body : linkage;
  Seq lambda_code =
    EndWithLinkage(
      MakeSeq(RegEnv, 0,
        Pair(OpSymbol(OpLambda),
        Pair(LabelRef(entry, mem), nil, mem), mem)), lambda_linkage, mem);

  Val bindings = nil;
  for (u32 i = 0; i < VecCount(ids); i++) {
    Val id = MakeSymbol(ids[i].lexeme, ids[i].length, mem);
    bindings =
      Pair(OpSymbol(OpConst),
      Pair(id,
      Pair(OpSymbol(OpDefine),
        bindings, mem), mem), mem);
  }
  FreeVec(ids);

  Seq body =
    AppendSeq(LabelSeq(entry, mem),
    AppendSeq(
      MakeSeq(0, RegEnv, bindings),
      CompileExpr(c, LinkReturn, PrecExpr), mem), mem);

  return
    AppendSeq(lambda_code,
    AppendSeq(body,
              LabelSeq(after_body, mem), mem), mem);
}

static Seq CompileApplication(Compiler *c, Linkage linkage, Token *args)
{
  return EmptySeq();
}

static Seq CompileGroup(Compiler *c, Linkage linkage)
{
  ExpectToken(c, TokenLParen);

  // save up a list of ID tokens in case it's a lambda
  Token *args = NULL;
  while (c->token.type == TokenID) {
    VecPush(args, c->token);
    AdvanceToken(c);
  }

  if (MatchToken(c, TokenRParen) && MatchToken(c, TokenArrow)) {
    return CompileLambda(c, linkage, args);
  }

  // not a lambda
  while (c->token.type != TokenRParen) {
    if (c->token.type == TokenEOF) {
      CompileError(c, "Unmatched parenthesis");
      return EmptySeq();
    }

    VecPush(args, c->token);
    AdvanceToken(c);
  }

  ExpectToken(c, TokenRParen);
  return CompileApplication(c, linkage, args);
}

// void CompileString(Compiler *compiler);
// void CompileLiteral(Compiler *compiler);
// void CompileSymbol(Compiler *compiler);
// void CompileVariable(Compiler *compiler);
// // void CompileGroup(Compiler *compiler);
// void CompileUnary(Compiler *compiler);
// void CompileInfixLeft(Compiler *compiler);
// void CompileInfixRight(Compiler *compiler);
// // void CompileLogic(Compiler *compiler);
// // void CompileDoBlock(Compiler *compiler);
// // void CompileIfBlock(Compiler *compiler);
// // void CompileCondBlock(Compiler *compiler);
// // void CompileLet(Compiler *compiler);
// // void CompileDef(Compiler *compiler);
// // void CompileAccess(Compiler *compiler);
// // void CompileList(Compiler *compiler);
// // void CompileTuple(Compiler *compiler);
// // void CompileMap(Compiler *compiler);
// // void CompileModuleName(Compiler *compiler);

// static ParseRule rules[] = {
//   [TokenEOF]          = {NULL, NULL, PrecNone},
//   [TokenError]        = {NULL, NULL, PrecNone},
//   // [TokenLet]          = {&CompileLet, NULL, PrecNone},
//   [TokenComma]        = {NULL, NULL, PrecNone},
//   [TokenEqual]        = {NULL, NULL, PrecNone},
//   // [TokenDef]          = {&CompileDef, NULL, PrecNone},
//   // [TokenLParen]       = {&CompileGroup, NULL, PrecNone},
//   [TokenRParen]       = {NULL, NULL, PrecNone},
//   [TokenID]           = {&CompileVariable, NULL, PrecNone},
//   [TokenArrow]        = {NULL, NULL, PrecNone},
//   // [TokenAnd]          = {NULL, &CompileLogic, PrecLogic},
//   // [TokenOr]           = {NULL, &CompileLogic, PrecLogic},
//   [TokenEqualEqual]   = {NULL, &CompileInfixLeft, PrecEqual},
//   [TokenNotEqual]     = {NULL, &CompileInfixLeft, PrecEqual},
//   [TokenGreater]      = {NULL, &CompileInfixLeft, PrecCompare},
//   [TokenGreaterEqual] = {NULL, &CompileInfixLeft, PrecCompare},
//   [TokenLess]         = {NULL, &CompileInfixLeft, PrecCompare},
//   [TokenLessEqual]    = {NULL, &CompileInfixLeft, PrecCompare},
//   [TokenIn]           = {NULL, &CompileInfixLeft, PrecMember},
//   [TokenPipe]         = {NULL, &CompileInfixRight, PrecPair},
//   [TokenPlus]         = {NULL, &CompileInfixLeft, PrecSum},
//   [TokenMinus]        = {&CompileUnary, &CompileInfixLeft, PrecSum},
//   [TokenStar]         = {NULL, &CompileInfixLeft, PrecProduct},
//   [TokenSlash]        = {NULL, &CompileInfixLeft, PrecProduct},
//   [TokenNot]          = {&CompileUnary, NULL, PrecNone},
//   [TokenNum]          = {&CompileNum, NULL, PrecNone},
//   [TokenString]       = {&CompileString, NULL, PrecNone},
//   [TokenTrue]         = {&CompileLiteral, NULL, PrecNone},
//   [TokenFalse]        = {&CompileLiteral, NULL, PrecNone},
//   [TokenNil]          = {&CompileLiteral, NULL, PrecNone},
//   [TokenColon]        = {&CompileSymbol, NULL, PrecNone},
//   // [TokenDot]          = {&CompileAccess, NULL, PrecNone},
//   // [TokenDo]           = {&CompileDoBlock, NULL, PrecNone},
//   [TokenEnd]          = {NULL, NULL, PrecNone},
//   // [TokenIf]           = {&CompileIfBlock, NULL, PrecNone},
//   [TokenElse]         = {NULL, NULL, PrecNone},
//   // [TokenCond]         = {&CompileCondBlock, NULL, PrecNone},
//   // [TokenLBracket]     = {&CompileList, NULL, PrecNone},
//   [TokenRBracket]     = {NULL, NULL, PrecNone},
//   // [TokenHashBracket]  = {&CompileTuple, NULL, PrecNone},
//   // [TokenLBrace]       = {&CompileMap, NULL, PrecNone},
//   [TokenRBrace]       = {NULL, NULL, PrecNone},
//   [TokenNewline]      = {&AdvanceToken, NULL, PrecNone},
//   // [TokenModule]       = {&CompileModuleName, NULL, PrecNone},
// };

// void DebugCompile(char *message)
// {
//   Print(message);
// }

// bool Expect(bool assertion, char *message)
// {
//   if (!assertion) {
//     Print(message);
//     Print("\n");
//   }
//   return assertion;
// }

// bool ExpectToken(Compiler *compiler, TokenType type, char *message)
// {
//   if (compiler->token.type != type) {
//     CompileError(compiler, message);
//     return false;
//   }
//   return true;
// }

// void CompileExpr(Compiler *compiler, Precedence precedence)
// {
//   if (compiler->error) return;

//   ParseRule rule = rules[compiler->token.type];

//   if (rule.prefix == NULL) {
//     CompileError(compiler, "Expected expression");
//     return;
//   }

//   rule.prefix(compiler);

//   rule = rules[compiler->token.type];
//   while (rule.infix && rule.precedence >= precedence) {
//     rule.infix(compiler);
//     rule = rules[compiler->token.type];
//   }
// }

// // void CompileStatement(Compiler *compiler)
// // {
// //   if (compiler->error) return;
// //   SkipNewlines(compiler);

// //   Print("Compiling statement\n");

// //   u32 num_exprs = 0;
// //   while (compiler->token.type != TokenEOF && compiler->token.type != TokenNewline) {
// //     num_exprs++;
// //     CompileExpr(compiler, PrecExpr);
// //   }

// //   if (num_exprs > 1) {
// //     PushByte(compiler->chunk, OpLink);
// //     u32 link = VecCount(compiler->chunk->data);
// //     PushByte(compiler->chunk, 0);
// //     PushByte(compiler->chunk, OpApply);
// //     PushByte(compiler->chunk, num_exprs);
// //     compiler->chunk->data[link] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   }

// //   SkipNewlines(compiler);
// // }

// //   PushByte(&chunk, OpHalt);

// //   return *compiler.chunk;
// // }

// // u32 CompileModule(char *source, Chunk *chunk)
// // {
// //   Compiler compiler;
// //   InitCompiler(&compiler, chunk, &source);

// //   // jump past module body
// //   PushByte(compiler.chunk, OpJump);
// //   u32 patch = VecCount(compiler.chunk->data);
// //   PushByte(compiler.chunk, 0); // patched later

// //   u32 position = VecCount(compiler.chunk->data);

// //   // module body
// //   while (compiler.token.type != TokenEOF) {
// //     CompileStatement(&compiler);
// //     PushByte(chunk, OpPop);
// //   }
// //   PushByte(chunk, OpConst);
// //   PushByte(chunk, PushConst(chunk, compiler.mod_name));
// //   PushByte(chunk, OpDefMod);
// //   PushByte(chunk, OpReturn);

// //   // after module
// //   chunk->data[patch] = PushConst(chunk, IntVal(VecCount(chunk->data)));
// //   PushByte(chunk, OpLambda);
// //   PushByte(chunk, PushConst(compiler.chunk, IntVal(patch+1)));

// //   return position;
// // }

// void CompileID(Compiler *compiler, Token id)
// {
//   Val sym = MakeSymbol(id.lexeme, id.length, &compiler->chunk->constants);
//   PushByte(compiler->chunk, OpConst);
//   PushByte(compiler->chunk, PushConst(compiler->chunk, sym));
// }

// void CompileNum(Compiler *compiler)
// {
//   DebugCompile("Compile Num\n");
//   PushByte(compiler->chunk, OpConst);
//   PushByte(compiler->chunk, PushConst(compiler->chunk, compiler->token.value));
//   AdvanceToken(compiler);
// }

// void CompileString(Compiler *compiler)
// {
//   DebugCompile("Compile String\n");
//   CompileID(compiler, compiler->token);
//   PushByte(compiler->chunk, OpStr);
//   AdvanceToken(compiler);
// }

// void CompileLiteral(Compiler *compiler)
// {
//   DebugCompile("Compile Literal\n");
//   switch (compiler->token.type) {
//   case TokenTrue:
//     PushByte(compiler->chunk, OpTrue);
//     break;
//   case TokenFalse:
//     PushByte(compiler->chunk, OpFalse);
//     break;
//   case TokenNil:
//     PushByte(compiler->chunk, OpNil);
//     break;
//   default: break;
//   }
//   AdvanceToken(compiler);
// }

// void CompileSymbol(Compiler *compiler)
// {
//   DebugCompile("Compile Symbol\n");
//   AdvanceToken(compiler);
//   CompileID(compiler, compiler->token);
//   AdvanceToken(compiler);
// }

// // void CompileLambda(Compiler *compiler, Token *ids)
// // {
// //   // jump past lambda body
// //   PushByte(compiler->chunk, OpJump);
// //   u32 patch = VecCount(compiler->chunk->data);
// //   PushByte(compiler->chunk, 0); // patched later

// //   // lambda body
// //   // args are on the stack; define each param
// //   for (u32 i = VecCount(ids); i > 0; i--) {
// //     Val sym = MakeSymbol(ids[i-1].lexeme, ids[i-1].length, &compiler->chunk->constants);
// //     PushByte(compiler->chunk, OpConst);
// //     PushByte(compiler->chunk, PushConst(compiler->chunk, sym));
// //     PushByte(compiler->chunk, OpDefine);
// //     PushByte(compiler->chunk, OpPop);
// //   }
// //   PushByte(compiler->chunk, OpPop); // pop the lambda from the stack

// //   CompileExpr(compiler, PrecExpr);
// //   PushByte(compiler->chunk, OpReturn);

// //   // after body
// //   // patch jump, create lambda
// //   compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   PushByte(compiler->chunk, OpLambda);
// //   PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(patch+1)));
// // }

// void CompileVariable(Compiler *compiler)
// {
//   DebugCompile("Compile Variable\n");
//   CompileID(compiler, compiler->token);
//   PushByte(compiler->chunk, OpLookup);
//   AdvanceToken(compiler);
// }

// // void CompileGroup(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   // try to store up IDs in case it's a lambda
// //   Token *ids = NULL;
// //   while (compiler->token.type == TokenID) {
// //     VecPush(ids, compiler->token);
// //     AdvanceToken(compiler);
// //     SkipNewlines(compiler);
// //   }

// //   if (compiler->token.type == TokenRParen) {
// //     AdvanceToken(compiler);
// //     if (compiler->token.type == TokenArrow) {
// //       AdvanceToken(compiler);
// //       SkipNewlines(compiler);
// //       CompileLambda(compiler, ids);
// //       FreeVec(ids);
// //       return;
// //     }

// //     // just a list of ids
// //     for (u32 i = 0; i < VecCount(ids); i++) {
// //       CompileID(compiler, ids[i]);
// //       PushByte(compiler->chunk, OpLookup);
// //     }
// //     PushByte(compiler->chunk, OpLink);
// //     u32 link = VecCount(compiler->chunk->data);
// //     PushByte(compiler->chunk, 0);
// //     PushByte(compiler->chunk, OpApply);
// //     PushByte(compiler->chunk, VecCount(ids));
// //     FreeVec(ids);
// //     compiler->chunk->data[link] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //     return;
// //   }

// //   // not a list of ids, just an expression
// //   for (u32 i = 0; i < VecCount(ids); i++) {
// //     CompileID(compiler, ids[i]);
// //     PushByte(compiler->chunk, OpLookup);
// //   }
// //   u8 num_params = VecCount(ids);
// //   FreeVec(ids);

// //   while (compiler->token.type != TokenRParen) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     num_params++;
// //     CompileExpr(compiler, PrecExpr);
// //   }

// //   PushByte(compiler->chunk, OpLink);
// //   u32 link = VecCount(compiler->chunk->data);
// //   PushByte(compiler->chunk, 0);
// //   PushByte(compiler->chunk, OpApply);
// //   PushByte(compiler->chunk, num_params);
// //   compiler->chunk->data[link] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   AdvanceToken(compiler);
// // }

// void CompileUnary(Compiler *compiler)
// {
//   DebugCompile("Compile Unary\n");
//   TokenType op = compiler->token.type;

//   AdvanceToken(compiler);
//   CompileExpr(compiler, PrecUnary);

//   switch (op) {
//   case TokenNot:
//     PushByte(compiler->chunk, OpNot);
//     break;
//   case TokenMinus:
//     PushByte(compiler->chunk, OpNeg);
//     break;
//   default: break;
//   }
// }

// void CompileInfixLeft(Compiler *compiler)
// {
//   DebugCompile("Compile Left\n");
//   TokenType op = compiler->token.type;
//   u32 prec = rules[compiler->token.type].precedence + 1;
//   AdvanceToken(compiler);
//   SkipNewlines(compiler);
//   CompileExpr(compiler, prec);

//   switch (op) {
//   case TokenEqualEqual:
//     PushByte(compiler->chunk, OpEq);
//     break;
//   case TokenNotEqual:
//     PushByte(compiler->chunk, OpEq);
//     PushByte(compiler->chunk, OpNot);
//     break;
//   case TokenGreater:
//     PushByte(compiler->chunk, OpGt);
//     break;
//   case TokenGreaterEqual:
//     PushByte(compiler->chunk, OpLt);
//     PushByte(compiler->chunk, OpNot);
//     break;
//   case TokenLess:
//     PushByte(compiler->chunk, OpLt);
//     break;
//   case TokenLessEqual:
//     PushByte(compiler->chunk, OpGt);
//     PushByte(compiler->chunk, OpNot);
//     break;
//   case TokenIn:
//     PushByte(compiler->chunk, OpIn);
//     break;
//   case TokenPlus:
//     PushByte(compiler->chunk, OpAdd);
//     break;
//   case TokenMinus:
//     PushByte(compiler->chunk, OpSub);
//     break;
//   case TokenStar:
//     PushByte(compiler->chunk, OpMul);
//     break;
//   case TokenSlash:
//     PushByte(compiler->chunk, OpDiv);
//     break;
//   default:
//     CompileError(compiler, "Unknown left-associative operator");
//     break;
//   }
// }

// void CompileInfixRight(Compiler *compiler)
// {
//   DebugCompile("Compile Right\n");
//   TokenType op = compiler->token.type;
//   u32 prec = rules[compiler->token.type].precedence;
//   AdvanceToken(compiler);
//   SkipNewlines(compiler);
//   CompileExpr(compiler, prec);

//   switch (op) {
//   case TokenPipe:
//     PushByte(compiler->chunk, OpPair);
//     break;
//   default:
//     CompileError(compiler, "Unknown right-associative operator");
//     break;
//   }
// }

// // void CompileLogic(Compiler *compiler)
// // {
// //   TokenType op = compiler->token.type;
// //   u32 prec = rules[compiler->token.type].precedence;
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   if (op == TokenAnd) {
// //     PushByte(compiler->chunk, OpBranchF);
// //   } else {
// //     PushByte(compiler->chunk, OpBranch);
// //   }

// //   u32 patch = VecCount(compiler->chunk->data);
// //   PushByte(compiler->chunk, 0);

// //   PushByte(compiler->chunk, OpPop);
// //   CompileExpr(compiler, prec);

// //   compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// // }

// // void CompileDoBlock(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   while (compiler->token.type != TokenEnd) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     CompileStatement(compiler);
// //   }
// //   AdvanceToken(compiler);
// // }

// // void CompileIfBlock(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   CompileExpr(compiler, PrecExpr);

// //   PushByte(compiler->chunk, OpBranchF);
// //   u32 branch_false = VecCount(compiler->chunk->data);
// //   PushByte(compiler->chunk, 0);

// //   if (compiler->token.type != TokenDo) {
// //     CompileError(compiler, "Expected \"do\" after condition");
// //     return;
// //   }
// //   AdvanceToken(compiler);

// //   // true branch
// //   PushByte(compiler->chunk, OpPop); // discard condition
// //   while (compiler->token.type != TokenEnd && compiler->token.type != TokenElse) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     CompileStatement(compiler);
// //     PushByte(compiler->chunk, OpPop);
// //   }
// //   RewindVec(compiler->chunk->data, 1);

// //   // jump past false branch
// //   PushByte(compiler->chunk, OpJump);
// //   u32 jump_after = VecCount(compiler->chunk->data);
// //   PushByte(compiler->chunk, 0);

// //   // false branch
// //   compiler->chunk->data[branch_false] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   PushByte(compiler->chunk, OpPop); // discard condition
// //   if (compiler->token.type == TokenElse) {
// //     AdvanceToken(compiler);
// //     while (compiler->token.type != TokenEnd) {
// //       if (compiler->token.type == TokenEOF) {
// //         CompileError(compiler, "Unexpected end of file");
// //         return;
// //       }
// //       CompileStatement(compiler);
// //       PushByte(compiler->chunk, OpPop);
// //     }
// //     RewindVec(compiler->chunk->data, 1);
// //   } else {
// //     // default else
// //     PushByte(compiler->chunk, OpNil);
// //   }

// //   compiler->chunk->data[jump_after] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   AdvanceToken(compiler);
// // }

// // void CompileCondBlock(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   if (compiler->token.type != TokenDo) {
// //     CompileError(compiler, "Expected \"do\" after \"cond\"");
// //     return;
// //   }
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   u32 *patches = NULL;
// //   while (compiler->token.type != TokenEnd) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }

// //     // condition
// //     CompileExpr(compiler, PrecLambda + 1); // don't allow lambdas in condition
// //     SkipNewlines(compiler);
// //     if (compiler->token.type != TokenArrow) {
// //       CompileError(compiler, "Expected \"->\" after clause condition");
// //       return;
// //     }
// //     AdvanceToken(compiler);
// //     SkipNewlines(compiler);

// //     // skip consequent when false
// //     PushByte(compiler->chunk, OpBranchF);
// //     u32 patch = VecCount(compiler->chunk->data);
// //     PushByte(compiler->chunk, 0);

// //     // consequent
// //     CompileStatement(compiler);

// //     // jump to after cond
// //     PushByte(compiler->chunk, OpJump);
// //     VecPush(patches, VecCount(compiler->chunk->data));
// //     PushByte(compiler->chunk, 0);

// //     compiler->chunk->data[patch] = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   }

// //   u8 after_cond = PushConst(compiler->chunk, IntVal(VecCount(compiler->chunk->data)));
// //   for (u32 i = 0; i < VecCount(patches); i++) {
// //     compiler->chunk->data[patches[i]] = after_cond;
// //   }
// //   FreeVec(patches);

// //   AdvanceToken(compiler);
// // }

// // void CompileAssign(Compiler *compiler)
// // {
// //   if (compiler->token.type != TokenID) {
// //     CompileError(compiler, "Expected identifier");
// //     return;
// //   }

// //   Token id = compiler->token;
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   if (compiler->token.type != TokenEqual) {
// //     CompileError(compiler, "Expected \"=\" after identifier");
// //     return;
// //   }
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   CompileExpr(compiler, PrecExpr);
// //   CompileID(compiler, id);
// //   PushByte(compiler->chunk, OpDefine);
// // }

// // void CompileLet(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);

// //   CompileAssign(compiler);
// //   while (compiler->token.type == TokenComma) {
// //     PushByte(compiler->chunk, OpPop);
// //     AdvanceToken(compiler);
// //     SkipNewlines(compiler);
// //     CompileAssign(compiler);
// //   }
// // }

// // void CompileDef(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   if (compiler->token.type == TokenLParen) {
// //     AdvanceToken(compiler);
// //     if (compiler->token.type != TokenID) {
// //       CompileError(compiler, "Expected identifier");
// //       return;
// //     }
// //     Token id = compiler->token;
// //     AdvanceToken(compiler);

// //     Token *params = NULL;
// //     while (compiler->token.type == TokenID) {
// //       VecPush(params, compiler->token);
// //       AdvanceToken(compiler);
// //       SkipNewlines(compiler);
// //     }
// //     if (compiler->token.type != TokenRParen) {
// //       CompileError(compiler, "Expected \")\"");
// //       return;
// //     }
// //     AdvanceToken(compiler);

// //     CompileLambda(compiler, params);
// //     CompileID(compiler, id);
// //     PushByte(compiler->chunk, OpDefine);
// //   } else if (compiler->token.type == TokenID) {
// //     Token id = compiler->token;
// //     AdvanceToken(compiler);
// //     CompileExpr(compiler, PrecExpr);
// //     CompileID(compiler, id);
// //     PushByte(compiler->chunk, OpDefine);
// //   }
// // }

// // void CompileAccess(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   if (compiler->token.type != TokenID) {
// //     CompileError(compiler, "Expected identifier");
// //     return;
// //   }
// //   CompileID(compiler, compiler->token);
// //   AdvanceToken(compiler);
// //   PushByte(compiler->chunk, OpAccess);
// // }

// // void CompileList(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);
// //   u32 num_items = 0;
// //   while (compiler->token.type != TokenRBracket) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     CompileExpr(compiler, PrecExpr);
// //     num_items++;
// //     if (compiler->token.type == TokenComma) AdvanceToken(compiler);
// //     SkipNewlines(compiler);
// //   }
// //   AdvanceToken(compiler);
// //   PushByte(compiler->chunk, OpList);
// //   PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
// // }

// // void CompileTuple(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);
// //   u32 num_items = 0;
// //   while (compiler->token.type != TokenRBracket) {
// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     CompileExpr(compiler, PrecExpr);
// //     num_items++;
// //     if (compiler->token.type == TokenComma) AdvanceToken(compiler);
// //     SkipNewlines(compiler);
// //   }
// //   AdvanceToken(compiler);
// //   PushByte(compiler->chunk, OpTuple);
// //   PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
// // }

// // void CompileMap(Compiler *compiler)
// // {
// //   AdvanceToken(compiler);
// //   SkipNewlines(compiler);
// //   u32 num_items = 0;
// //   while (compiler->token.type != TokenRBrace) {
// //     if (compiler->token.type != TokenID) {
// //       CompileError(compiler, "Expected identifier");
// //       return;
// //     }
// //     Token id = compiler->token;
// //     AdvanceToken(compiler);
// //     if (compiler->token.type != TokenColon) {
// //       CompileError(compiler, "Expected \":\"");
// //       return;
// //     }
// //     AdvanceToken(compiler);
// //     SkipNewlines(compiler);

// //     if (compiler->token.type == TokenEOF) {
// //       CompileError(compiler, "Unexpected end of file");
// //       return;
// //     }
// //     CompileExpr(compiler, PrecExpr);
// //     CompileID(compiler, id);
// //     num_items++;

// //     if (compiler->token.type == TokenComma) AdvanceToken(compiler);
// //     SkipNewlines(compiler);
// //   }
// //   AdvanceToken(compiler);
// //   PushByte(compiler->chunk, OpMap);
// //   PushByte(compiler->chunk, PushConst(compiler->chunk, IntVal(num_items)));
// // }
