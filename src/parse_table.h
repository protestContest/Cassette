/*
  0 program     →  stmts
  0 program     →  $
  1 stmts       →  stmts
  1 stmts       →  stmt
  1 stmts       →  stmt_end
  2 stmts       →  stmt_end
  3 stmts       →  ε
  4 stmt        →  let_stmt
  5 stmt        →  def_stmt
  6 stmt        →  call
  7 stmt_end    →  NL
  8 stmt_end    →  $
  9 let_stmt    →  "let"
  9 let_stmt    →  assigns
 10 assigns     →  assigns
 10 assigns     →  ","
 10 assigns     →  assign
 11 assigns     →  assign
 12 assign      →  ID
 12 assign      →  "="
 12 assign      →  call
 13 def_stmt    →  "def"
 13 def_stmt    →  "("
 13 def_stmt    →  id_list
 13 def_stmt    →  ")"
 13 def_stmt    →  arg
 14 id_list     →  id_list
 14 id_list     →  ID
 15 id_list     →  ID
 16 call        →  call
 16 call        →  arg
 17 call        →  arg
 18 arg         →  lambda
 19 lambda      →  ID
 19 lambda      →  "->"
 19 lambda      →  lambda
 20 lambda      →  group
 20 lambda      →  "->"
 20 lambda      →  lambda
 21 lambda      →  "("
 21 lambda      →  ")"
 21 lambda      →  "->"
 21 lambda      →  lambda
 22 lambda      →  logic
 23 logic       →  logic
 23 logic       →  "and"
 23 logic       →  equals
 24 logic       →  logic
 24 logic       →  "or"
 24 logic       →  equals
 25 logic       →  equals
 26 equals      →  compare
 26 equals      →  "=="
 26 equals      →  compare
 27 equals      →  compare
 27 equals      →  "!="
 27 equals      →  compare
 28 equals      →  compare
 29 compare     →  member
 29 compare     →  ">"
 29 compare     →  member
 30 compare     →  member
 30 compare     →  ">="
 30 compare     →  member
 31 compare     →  member
 31 compare     →  "<"
 31 compare     →  member
 32 compare     →  member
 32 compare     →  "<="
 32 compare     →  member
 33 compare     →  member
 34 member      →  sum
 34 member      →  "in"
 34 member      →  sum
 35 member      →  sum
 36 sum         →  sum
 36 sum         →  "+"
 36 sum         →  product
 37 sum         →  sum
 37 sum         →  "-"
 37 sum         →  product
 38 sum         →  product
 39 product     →  product
 39 product     →  "*"
 39 product     →  unary
 40 product     →  product
 40 product     →  "/"
 40 product     →  unary
 41 product     →  unary
 42 unary       →  "not"
 42 unary       →  primary
 43 unary       →  primary
 44 primary     →  NUM
 45 primary     →  ID
 46 primary     →  STR
 47 primary     →  literal
 48 primary     →  symbol
 49 primary     →  access
 50 primary     →  group
 51 primary     →  block
 52 primary     →  object
 53 literal     →  "true"
 54 literal     →  "false"
 55 literal     →  "nil"
 56 symbol      →  ":"
 56 symbol      →  ID
 57 access      →  access
 57 access      →  "."
 57 access      →  ID
 58 access      →  ID
 58 access      →  "."
 58 access      →  ID
 59 group       →  "("
 59 group       →  call
 59 group       →  ")"
 60 block       →  do_block
 61 block       →  if_block
 62 block       →  cond_block
 63 do_block    →  "do"
 63 do_block    →  stmts
 63 do_block    →  "end"
 64 if_block    →  "if"
 64 if_block    →  arg
 64 if_block    →  do_else
 65 if_block    →  "if"
 65 if_block    →  arg
 65 if_block    →  do_block
 65 if_block    →  nothing
 66 do_else     →  "do"
 66 do_else     →  stmts
 66 do_else     →  "else"
 66 do_else     →  stmts
 66 do_else     →  "end"
 67 cond_block  →  "cond"
 67 cond_block  →  "do"
 67 cond_block  →  clauses
 67 cond_block  →  "end"
 68 clauses     →  clauses
 68 clauses     →  clause
 69 clauses     →  NL
 70 clauses     →  ε
 71 clause      →  logic
 71 clause      →  "->"
 71 clause      →  call
 71 clause      →  NL
 72 object      →  list
 73 object      →  tuple
 74 object      →  map
 75 list        →  "["
 75 list        →  items
 75 list        →  "]"
 76 tuple       →  "#["
 76 tuple       →  items
 76 tuple       →  "]"
 77 map         →  "{"
 77 map         →  entries
 77 map         →  "}"
 78 items       →  items
 78 items       →  item
 79 items       →  ε
 80 item        →  arg
 80 item        →  opt_comma
 81 entries     →  entries
 81 entries     →  entry
 82 entries     →  ε
 83 entry       →  ID
 83 entry       →  ":"
 83 entry       →  arg
 83 entry       →  opt_comma
 84 opt_comma   →  ","
 85 opt_comma   →  NL
 86 opt_comma   →  ε
 87 nothing     →  ε
*/

enum {
  ParseSymProgram    =  0,
  ParseSymStmts      =  1,
  ParseSymStmt       =  2,
  ParseSymStmtEnd    =  3,
  ParseSymLetStmt    =  4,
  ParseSymAssigns    =  5,
  ParseSymAssign     =  6,
  ParseSymDefStmt    =  7,
  ParseSymIdList     =  8,
  ParseSymCall       =  9,
  ParseSymArg        = 10,
  ParseSymLambda     = 11,
  ParseSymLogic      = 12,
  ParseSymEquals     = 13,
  ParseSymCompare    = 14,
  ParseSymMember     = 15,
  ParseSymSum        = 16,
  ParseSymProduct    = 17,
  ParseSymUnary      = 18,
  ParseSymPrimary    = 19,
  ParseSymLiteral    = 20,
  ParseSymSymbol     = 21,
  ParseSymAccess     = 22,
  ParseSymGroup      = 23,
  ParseSymBlock      = 24,
  ParseSymDoBlock    = 25,
  ParseSymIfBlock    = 26,
  ParseSymDoElse     = 27,
  ParseSymCondBlock  = 28,
  ParseSymClauses    = 29,
  ParseSymClause     = 30,
  ParseSymObject     = 31,
  ParseSymList       = 32,
  ParseSymTuple      = 33,
  ParseSymMap        = 34,
  ParseSymItems      = 35,
  ParseSymItem       = 36,
  ParseSymEntries    = 37,
  ParseSymEntry      = 38,
  ParseSymOptComma   = 39,
  ParseSymNothing    = 40,
  ParseSymEOF        = 41,
  ParseSymNL         = 42,
  ParseSymLet        = 43,
  ParseSymComma      = 44,
  ParseSymID         = 45,
  ParseSymEqual      = 46,
  ParseSymDef        = 47,
  ParseSymLParen     = 48,
  ParseSymRParen     = 49,
  ParseSymArrow      = 50,
  ParseSymAnd        = 51,
  ParseSymOr         = 52,
  ParseSymEqualEqual = 53,
  ParseSymNotEqual   = 54,
  ParseSymGreaterThan = 55,
  ParseSymGreaterEqual = 56,
  ParseSymLessThan   = 57,
  ParseSymLessEqual  = 58,
  ParseSymIn         = 59,
  ParseSymPlus       = 60,
  ParseSymMinus      = 61,
  ParseSymStar       = 62,
  ParseSymSlash      = 63,
  ParseSymNot        = 64,
  ParseSymNUM        = 65,
  ParseSymSTR        = 66,
  ParseSymTrue       = 67,
  ParseSymFalse      = 68,
  ParseSymNil        = 69,
  ParseSymColon      = 70,
  ParseSymDot        = 71,
  ParseSymDo         = 72,
  ParseSymEnd        = 73,
  ParseSymIf         = 74,
  ParseSymElse       = 75,
  ParseSymCond       = 76,
  ParseSymLBracket   = 77,
  ParseSymRBracket   = 78,
  ParseSymHashBracket = 79,
  ParseSymLBrace     = 80,
  ParseSymRBrace     = 81,
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
