// #include "parse.h"
// #include "scan.h"
// #include "../runtime/mem.h"
// #include <univ/vec.h>
// #include <univ/io.h>

// #define DEBUG_COMPILE

// #ifdef DEBUG_COMPILE
// #define Debug(msg)  Print(msg), Print("\n")
// #else
// #define Debug(msg)  (void)0
// #endif

// #define CurToken(p) ((p)->token.type)

// void InitParser(Parser *p)
// {
//   InitSource(&p->source, NULL);
//   p->error = NULL;
// }

// void AdvanceToken(Parser *p)
// {
//   p->token = ScanToken(&p->source);
// }

// static void SkipNewlines(Parser *p)
// {
//   while (CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);
// }

// static void ExpectToken(Parser *p, TokenType type)
// {
//   if (p->token.type == type) {
//     AdvanceToken(p);
//   } else {
//     p->error = "Expected token";
//   }
// }

// static bool MatchToken(Parser *p, TokenType type)
// {
//   if (p->token.type == type) {
//     AdvanceToken(p);
//     return true;
//   }
//   return false;
// }

// typedef enum {
//   PARSE_ERROR,
//   PARSE_SHIFT,
//   PARSE_REDUCE,
//   PARSE_STOP,
// } ParseAction;

// typedef enum {
//   P_ID,
//   P_NUM,
//   P_LPAREN,
//   P_RPAREN,
//   P_PLUS,
//   P_SCRIPT,
//   P_EXPR,
//   P_INFIX,
//   P_CALL,
//   P_VAL,
//   P_ARGS
// } ParseSymbol;



// void ParseNext(Parser *p)
// {

// }

// bool Parse(Parser *p, char *src)
// {
//   p->error = NULL;
//   InitSource(&p->source, src);
//   AdvanceToken(p);
//   p->stack = NULL;
//   VecPush(p->stack, 0);

//   ParseNext(p);

//   return false;
// }

























// typedef enum {
//   PREC_NONE,
//   PREC_EXPR,
//   PREC_UNARY,
// } Precedence;

// typedef ASTNode (*ParseFn)(Parser *p);

// typedef struct {
//   ParseFn prefix;
//   ParseFn infix;
//   Precedence prec;
// } ParseRule;

// static ASTNode Number(Parser *p);
// static ASTNode Unary(Parser *p);

// static ParseRule rules[] = {
//   [TOKEN_EOF]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_ERROR]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_NUMBER]    = { Number,             NULL,               PREC_NONE },
//   [TOKEN_STRING]    = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_IDENT]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_LPAREN]    = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_RPAREN]    = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_LBRACKET]  = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_RBRACKET]  = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_LBRACE]    = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_RBRACE]    = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_DOT]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_STAR]      = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_SLASH]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_MINUS]     = { Unary,              NULL,               PREC_NONE },
//   [TOKEN_PLUS]      = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_BAR]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_GREATER]   = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_LESS]      = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_EQUAL]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_COMMA]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_COLON]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_NEWLINE]   = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_ARROW]     = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_AND]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_COND]      = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_DEF]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_DO]        = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_ELSE]      = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_END]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_IF]        = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_NOT]       = { NULL,               NULL,               PREC_NONE },
//   [TOKEN_OR]        = { NULL,               NULL,               PREC_NONE },
// };

// #define GetRule(p)  &rules[CurToken(p)]

// ASTNode ParsePrec(Parser *p, Precedence prec)
// {
//   if (p->error) return MakeNode(p->token, nil);

//   ParseRule *rule = GetRule(p);
//   if (!rule->prefix) {
//     p->error = "Bad prefix";
//     return MakeNode(p->token, nil);
//   }

//   ASTNode node = rule->prefix(p);

//   return node;
// }

// // ASTNode Parse(Parser *p, char *src)
// // {
// //   p->error = NULL;
// //   InitSource(&p->source, src);
// //   AdvanceToken(p);
// //   return (ASTNode){{TOKEN_ERROR, 0, 0, NULL, 0}, nil, NULL};
// //   return ParsePrec(p, PREC_EXPR);
// // }

// static ASTNode Number(Parser *p)
// {
//   Debug("Number");

//   Token token = p->token;
//   AdvanceToken(p);
//   u32 num = 0;

//   for (u32 i = 0; i < token.length; i++) {
//     if (token.lexeme[i] == '_') continue;
//     if (token.lexeme[i] == '.') {
//       float frac = 0.0;
//       float magn = 10.0;
//       for (u32 j = i+1; j < token.length; j++) {
//         if (token.lexeme[i] == '_') continue;
//         u32 digit = token.lexeme[j] - '0';
//         frac += (float)digit / magn;
//       }

//       float f = (float)num + frac;
//       return MakeNode(token, NumVal(f));
//     }

//     u32 digit = token.lexeme[i] - '0';
//     num = num*10 + digit;
//   }

//   Assert(IsInt(IntVal(num)));

//   return MakeNode(token, IntVal(num));
// }

// static ASTNode Unary(Parser *p)
// {
//   Debug("Unary");

//   ASTNode node = MakeNode(p->token, nil);
//   AdvanceToken(p);
//   return PushNode(node, ParsePrec(p, PREC_UNARY));
// }
