#pragma once
#include "mem/mem.h"
#include "lex.h"
#include "mem/symbols.h"
#include "result.h"

#define SymEOF            0x7FD0D457 /* *eof* */
#define SymID             0x7FD0FE1D /* *id* */
#define SymBangEqual      0x7FD05C4E /* != */
#define SymString         0x7FD0C953 /* " */
#define SymNewline        0x7FD0ACD7 /* \n */
#define SymHash           0x7FD082DB /* # */
#define SymPercent        0x7FD0E679 /* % */
#define SymAmpersand      0x7FD0283C /* & */
#define SymLParen         0x7FD0C1C9 /* ( */
#define SymRParen         0x7FD08B60 /* ) */
#define SymStar           0x7FD0A24B /* * */
#define SymPlus           0x7FD06AB0 /* + */
#define SymComma          0x7FD06673 /* , */
#define SymMinus          0x7FD0FBF9 /* - */
#define SymArrow          0x7FD00CB9 /* -> */
#define SymDot            0x7FD01A5F /* . */
#define SymSlash          0x7FD0A21C /* / */
#define SymNum            0x7FD0FB37 /* *num* */
#define SymColon          0x7FD0ED8B /* : */
#define SymLess           0x7FD01F00 /* < */
#define SymLessLess       0x7FD02101 /* << */
#define SymLessEqual      0x7FD001F2 /* <= */
#define SymLessGreater    0x7FD0C54B /* <> */
#define SymEqual          0x7FD03FF3 /* = */
#define SymEqualEqual     0x7FD05014 /* == */
#define SymGreater        0x7FD0FB4A /* > */
#define SymGreaterEqual   0x7FD0C966 /* >= */
#define SymGreaterGreater 0x7FD00DDF /* >> */
#define SymLBracket       0x7FD0A8DA /* [ */
#define SymRBracket       0x7FD0F739 /* ] */
#define SymCaret          0x7FD017FE /* ^ */
#define SymAnd            0x7FD09F13 /* and */
#define SymAs             0x7FD08A20 /* as */
#define SymCond           0x7FD0818B /* cond */
#define SymDef            0x7FD00914 /* def */
#define SymDo             0x7FD0FCF4 /* do */
#define SymElse           0x7FD0B8F8 /* else */
#define SymEnd            0x7FD0273A /* end */
#define SymFalse          0x7FD0F4C4 /* false */
#define SymIf             0x7FD093D3 /* if */
#define SymImport         0x7FD0B92B /* import */
#define SymIn             0x7FD08998 /* in */
#define SymLet            0x7FD0E6A4 /* let */
#define SymModule         0x7FD0502A /* module */
#define SymNil            0x7FD0AFC3 /* nil */
#define SymNot            0x7FD0CA20 /* not */
#define SymOr             0x7FD043B9 /* or */
#define SymTrue           0x7FD0E33F /* true */
#define SymLBrace         0x7FD09DE3 /* { */
#define SymBar            0x7FD01ADB /* | */
#define SymRBrace         0x7FD0CE87 /* } */
#define SymTilde          0x7FD073CF /* ~ */

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
