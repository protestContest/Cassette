#pragma once
#include "mem/mem.h"
#include "lex.h"
#include "mem/symbols.h"
#include "result.h"

#define SymEOF            0x7FDDD97B
#define SymID             0x7FD7F416
#define SymBangEqual      0x7FDD5EA0
#define SymString         0x7FD9C2F9
#define SymNewline        0x7FD0A901
#define SymHash           0x7FDF8CB8
#define SymPercent        0x7FD3EC31
#define SymAmpersand      0x7FDB22AB
#define SymLParen         0x7FD2CFBD
#define SymRParen         0x7FDA86A6
#define SymStar           0x7FD9AA08
#define SymPlus           0x7FD26F51
#define SymComma          0x7FD868CB
#define SymMinus          0x7FD9FF2D
#define SymArrow          0x7FDA0014
#define SymDot            0x7FD21FF7
#define SymSlash          0x7FDDA557
#define SymNum            0x7FD6FFA6
#define SymColon          0x7FD7E2FE
#define SymLess           0x7FDD1E23
#define SymLessLess       0x7FD72265
#define SymLessEqual      0x7FDE0FA1
#define SymLessGreater    0x7FD3C208
#define SymEqual          0x7FD433E7
#define SymEqualEqual     0x7FDC5428
#define SymGreater        0x7FD9FE4E
#define SymGreaterEqual   0x7FD7CF7B
#define SymGreaterGreater 0x7FDA02D2
#define SymLBracket       0x7FDCA2BF
#define SymRBracket       0x7FD4F959
#define SymCaret          0x7FDC1A43
#define SymAnd            0x7FDF9232
#define SymAs             0x7FD680A2
#define SymCond           0x7FD1863D
#define SymDef            0x7FD9082B
#define SymDo             0x7FDEFFDA
#define SymElse           0x7FD0BE10
#define SymEnd            0x7FD026BE
#define SymFalse          0x7FDAF256
#define SymIf             0x7FDE9E90
#define SymImport         0x7FD8B461
#define SymIn             0x7FD98FB0
#define SymLet            0x7FDAEAD0
#define SymModule         0x7FD258A9
#define SymNil            0x7FD5A26C
#define SymNot            0x7FDBCF3C
#define SymOr             0x7FD149E0
#define SymTrue           0x7FD7E0EC
#define SymLBrace         0x7FD991EE
#define SymBar            0x7FDA1FBF
#define SymRBrace         0x7FD3C948
#define SymTilde        0x7FD37984

typedef struct {
  char *filename;
  Lexer lex;
  Mem *mem;
  SymbolTable *symbols;
  Val imports;
} Parser;

void InitParser(Parser *p, Mem *mem, SymbolTable *symbols);

Result ParseModule(Parser *p);

Val NodeType(Val node, Mem *mem);
u32 NodePos(Val node, Mem *mem);
Val NodeExpr(Val node, Mem *mem);
Val TokenSym(TokenType type);
