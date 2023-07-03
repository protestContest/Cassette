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
 17 lambda      →  group "->" lambda
 18 lambda      →  logic
 19 logic       →  logic "and" equals
 20 logic       →  logic "or" equals
 21 logic       →  equals
 22 equals      →  compare "==" compare
 23 equals      →  compare "!=" compare
 24 equals      →  compare
 25 compare     →  member ">" member
 26 compare     →  member ">=" member
 27 compare     →  member "<" member
 28 compare     →  member "<=" member
 29 compare     →  member
 30 member      →  sum "in" sum
 31 member      →  sum
 32 sum         →  sum "+" product
 33 sum         →  sum "-" product
 34 sum         →  product
 35 product     →  product "*" unary
 36 product     →  product "/" unary
 37 product     →  unary
 38 unary       →  "not" primary
 39 unary       →  primary
 40 primary     →  NUM
 41 primary     →  ID
 42 primary     →  STR
 43 primary     →  literal
 44 primary     →  symbol
 45 primary     →  access
 46 primary     →  group
 47 primary     →  block
 48 primary     →  object
 49 literal     →  "true"
 50 literal     →  "false"
 51 literal     →  "nil"
 52 symbol      →  ":" ID
 53 access      →  access "." ID
 54 access      →  ID "." ID
 55 group       →  "(" call ")"
 56 block       →  do_block
 57 block       →  if_block
 58 block       →  cond_block
 59 do_block    →  "do" stmts "end"
 60 if_block    →  "if" arg do_else
 61 do_else     →  "do" stmts "else" stmts "end"
 62 cond_block  →  "cond" "do" clauses "end"
 63 clauses     →  clauses clause
 64 clauses     →  NL
 65 clauses     →  ε
 66 clause      →  logic "->" call NL
 67 object      →  list
 68 object      →  tuple
 69 object      →  map
 70 list        →  "[" items "]"
 71 tuple       →  "#[" items "]"
 72 map         →  "{" entries "}"
 73 items       →  items item
 74 items       →  ε
 75 item        →  arg opt_comma
 76 entries     →  entries entry
 77 entries     →  ε
 78 entry       →  ID ":" arg opt_comma
 79 opt_comma   →  ","
 80 opt_comma   →  NL
 81 opt_comma   →  ε
 82 nothing     →  ε
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
