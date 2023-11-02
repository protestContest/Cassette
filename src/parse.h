#pragma once
#include "mem.h"
#include "lex.h"
#include "symbols.h"
#include "compile.h"

#define SymEOF           0x7FD76FC6
#define SymID            0x7FD5263F
#define SymBangEqual     0x7FD1154E
#define SymString        0x7FD89F9A
#define SymNewline       0x7FD6FAEC
#define SymHash          0x7FD8A12D
#define SymPercent       0x7FD897BB
#define SymLParen        0x7FD88FDC
#define SymRParen        0x7FD8916F
#define SymStar          0x7FD89302
#define SymPlus          0x7FD89495
#define SymComma         0x7FD88990
#define SymMinus         0x7FD88B23
#define SymArrow         0x7FD2FEA7
#define SymDot           0x7FD88CB6
#define SymSlash         0x7FD88E49
#define SymNum           0x7FD132E0
#define SymColon         0x7FD879D2
#define SymLess          0x7FD87060
#define SymLessEqual     0x7FD8E267
#define SymEqual         0x7FD871F3
#define SymEqualEqual    0x7FDB274A
#define SymGreater       0x7FD87386
#define SymGreaterEqual  0x7FDE2F61
#define SymLBracket      0x7FD8ADC5
#define SymRBracket      0x7FD8A453
#define SymAnd           0x7FD93EBD
#define SymAs            0x7FDF415C
#define SymCond          0x7FDE98F4
#define SymDef           0x7FDF5543
#define SymDo            0x7FD26285
#define SymElse          0x7FDA4AF5
#define SymEnd           0x7FDBF771
#define SymFalse         0x7FDE2C6F
#define SymIf            0x7FDBB4EB
#define SymImport        0x7FDD73F1
#define SymIn            0x7FDBA853
#define SymLet           0x7FD87181
#define SymModule        0x7FDA9814
#define SymNil           0x7FD16673
#define SymNot           0x7FD287FD
#define SymOr            0x7FD74AA1
#define SymTrue          0x7FD9395C
#define SymLBrace        0x7FD8E025
#define SymBar           0x7FD8D520
#define SymRBrace        0x7FD8D6B3

#define ParseError       0x7FD257DC

typedef struct {
  Lexer lex;
  Mem *mem;
  SymbolTable *symbols;
} Parser;

void InitParser(Parser *p, Mem *mem, SymbolTable *symbols);

Val Parse(char *source, Parser *p);
Val ParseModule(char *filename, Parser *p);

Val TokenSym(TokenType type);
void MakeParseSyms(SymbolTable *symbols);
void PrintAST(Val ast, u32 level, Mem *mem, SymbolTable *symbols);
