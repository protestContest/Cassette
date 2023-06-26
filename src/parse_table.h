/*
  0 program     →  stmts $
  1 stmts       →  stmts stmt NL
  2 stmts       →  NL
  3 stmts       →  ε
  4 stmt        →  let_stmt
  5 stmt        →  def_stmt
  6 stmt        →  call
  7 let_stmt    →  "let" assigns
  8 assigns     →  assigns "," assign
  9 assigns     →  assign
 10 assign      →  ID "=" call
 11 def_stmt    →  "def" "(" id_list ")" arg
 12 id_list     →  id_list ID
 13 id_list     →  ID
 14 call        →  call arg
 15 call        →  arg
 16 arg         →  lambda
 17 lambda      →  ID "->" logic
 18 lambda      →  group "->" logic
 19 lambda      →  "(" ")" "->" logic
 20 lambda      →  logic
 21 logic       →  logic "and" equals
 22 logic       →  logic "or" equals
 23 logic       →  equals
 24 equals      →  compare "==" compare
 25 equals      →  compare "!=" compare
 26 equals      →  compare
 27 compare     →  member ">" member
 28 compare     →  member ">=" member
 29 compare     →  member "<" member
 30 compare     →  member "<=" member
 31 compare     →  member
 32 member      →  sum "in" sum
 33 member      →  sum
 34 sum         →  sum "+" product
 35 sum         →  sum "-" product
 36 sum         →  product
 37 product     →  product "*" unary
 38 product     →  product "/" unary
 39 product     →  unary
 40 unary       →  "not" primary
 41 unary       →  primary
 42 primary     →  NUM
 43 primary     →  ID
 44 primary     →  STR
 45 primary     →  literal
 46 primary     →  symbol
 47 primary     →  access
 48 primary     →  group
 49 primary     →  block
 50 primary     →  object
 51 literal     →  "true"
 52 literal     →  "false"
 53 literal     →  "nil"
 54 symbol      →  ":" ID
 55 access      →  access "." ID
 56 access      →  ID "." ID
 57 group       →  "(" call ")"
 58 block       →  do_block
 59 block       →  if_block
 60 block       →  cond_block
 61 do_block    →  "do" stmts "end"
 62 if_block    →  "if" arg do_else
 63 if_block    →  "if" arg do_block nothing
 64 do_else     →  "do" stmts "else" stmts "end"
 65 cond_block  →  "cond" "do" clauses "end"
 66 clauses     →  clauses clause
 67 clauses     →  NL
 68 clauses     →  ε
 69 clause      →  logic "->" call NL
 70 object      →  list
 71 object      →  tuple
 72 object      →  map
 73 list        →  "[" items "]"
 74 tuple       →  "#[" items "]"
 75 map         →  "{" entries "}"
 76 items       →  items item
 77 items       →  ε
 78 item        →  arg opt_comma
 79 entries     →  entries entry
 80 entries     →  ε
 81 entry       →  ID ":" arg opt_comma
 82 opt_comma   →  ","
 83 opt_comma   →  NL
 84 opt_comma   →  ε
 85 nothing     →  ε
*/

enum {
  ParseSymProgram    =  0,
  ParseSymStmts      =  1,
  ParseSymStmt       =  2,
  ParseSymLetStmt    =  3,
  ParseSymAssigns    =  4,
  ParseSymAssign     =  5,
  ParseSymDefStmt    =  6,
  ParseSymIdList     =  7,
  ParseSymCall       =  8,
  ParseSymArg        =  9,
  ParseSymLambda     = 10,
  ParseSymLogic      = 11,
  ParseSymEquals     = 12,
  ParseSymCompare    = 13,
  ParseSymMember     = 14,
  ParseSymSum        = 15,
  ParseSymProduct    = 16,
  ParseSymUnary      = 17,
  ParseSymPrimary    = 18,
  ParseSymLiteral    = 19,
  ParseSymSymbol     = 20,
  ParseSymAccess     = 21,
  ParseSymGroup      = 22,
  ParseSymBlock      = 23,
  ParseSymDoBlock    = 24,
  ParseSymIfBlock    = 25,
  ParseSymDoElse     = 26,
  ParseSymCondBlock  = 27,
  ParseSymClauses    = 28,
  ParseSymClause     = 29,
  ParseSymObject     = 30,
  ParseSymList       = 31,
  ParseSymTuple      = 32,
  ParseSymMap        = 33,
  ParseSymItems      = 34,
  ParseSymItem       = 35,
  ParseSymEntries    = 36,
  ParseSymEntry      = 37,
  ParseSymOptComma   = 38,
  ParseSymNothing    = 39,
  ParseSymEOF        = 40,
  ParseSymNL         = 41,
  ParseSymLet        = 42,
  ParseSymComma      = 43,
  ParseSymID         = 44,
  ParseSymEqual      = 45,
  ParseSymDef        = 46,
  ParseSymLParen     = 47,
  ParseSymRParen     = 48,
  ParseSymArrow      = 49,
  ParseSymAnd        = 50,
  ParseSymOr         = 51,
  ParseSymEqualEqual = 52,
  ParseSymNotEqual   = 53,
  ParseSymGreaterThan = 54,
  ParseSymGreaterEqual = 55,
  ParseSymLessThan   = 56,
  ParseSymLessEqual  = 57,
  ParseSymIn         = 58,
  ParseSymPlus       = 59,
  ParseSymMinus      = 60,
  ParseSymStar       = 61,
  ParseSymSlash      = 62,
  ParseSymNot        = 63,
  ParseSymNUM        = 64,
  ParseSymSTR        = 65,
  ParseSymTrue       = 66,
  ParseSymFalse      = 67,
  ParseSymNil        = 68,
  ParseSymColon      = 69,
  ParseSymDot        = 70,
  ParseSymDo         = 71,
  ParseSymEnd        = 72,
  ParseSymIf         = 73,
  ParseSymElse       = 74,
  ParseSymCond       = 75,
  ParseSymLBracket   = 76,
  ParseSymRBracket   = 77,
  ParseSymHashBracket = 78,
  ParseSymLBrace     = 79,
  ParseSymRBrace     = 80,
NUM_SYMBOLS
};

i32 GetParseAction(i32 state, u32 sym);
i32 GetParseGoto(i32 state, u32 sym);
i32 GetParseReduction(i32 state);
i32 GetReductionNum(i32 state);
i32 NumLiterals(void);
i32 *GetLiterals(void);
char *ParseSymbolName(i32 sym);
bool IsParseError(i32 action);
);
;
 *GetLiterals(void);
char *ParseSymbolName(i32 sym);
bool IsParseError(i32 action);
20,
  ParseSymLiteral    = 21,
  ParseSymSymbol     = 22,
  ParseSymAccess     = 23,
  ParseSymGroup      = 24,
  ParseSymBlock      = 25,
  ParseSymDoBlock    = 26,
  ParseSymIfBlock    = 27,
  ParseSymDoElse     = 28,
  ParseSymCondBlock  = 29,
  ParseSymClauses    = 30,
  ParseSymClause     = 31,
  ParseSymObject     = 32,
  ParseSymList       = 33,
  ParseSymTuple      = 34,
  ParseSymMap        = 35,
  ParseSymItems      = 36,
  ParseSymItem       = 37,
  ParseSymEntries    = 38,
  ParseSymEntry      = 39,
  ParseSymOptComma   = 40,
  ParseSymNothing    = 41,
  ParseSymEOF        = 42,
  ParseSymNL         = 43,
  ParseSymLet        = 44,
  ParseSymComma      = 45,
  ParseSymID         = 46,
  ParseSymEqual      = 47,
  ParseSymDef        = 48,
  ParseSymLParen     = 49,
  ParseSymRParen     = 50,
  ParseSymArrow      = 51,
  ParseSymAnd        = 52,
  ParseSymOr         = 53,
  ParseSymEqualEqual = 54,
  ParseSymNotEqual   = 55,
  ParseSymGreaterThan = 56,
  ParseSymGreaterEqual = 57,
  ParseSymLessThan   = 58,
  ParseSymLessEqual  = 59,
  ParseSymIn         = 60,
  ParseSymPlus       = 61,
  ParseSymMinus      = 62,
  ParseSymStar       = 63,
  ParseSymSlash      = 64,
  ParseSymNot        = 65,
  ParseSymNUM        = 66,
  ParseSymSTR        = 67,
  ParseSymTrue       = 68,
  ParseSymFalse      = 69,
  ParseSymNil        = 70,
  ParseSymColon      = 71,
  ParseSymDot        = 72,
  ParseSymDo         = 73,
  ParseSymEnd        = 74,
  ParseSymIf         = 75,
  ParseSymElse       = 76,
  ParseSymCond       = 77,
  ParseSymLBracket   = 78,
  ParseSymRBracket   = 79,
  ParseSymHashBracket = 80,
  ParseSymLBrace     = 81,
  ParseSymRBrace     = 82,
NUM_SYMBOLS
};

i32 GetParseAction(i32 state, u32 sym);
i32 GetParseGoto(i32 state, u32 sym);
i32 GetParseReduction(i32 state);
i32 GetReductionNum(i32 state);
i32 NumLiterals(void);
i32 *GetLiterals(void);
char *ParseSymbolName(i32 sym);
bool IsParseError(i32 action);
