/*
program      →  "module" $
module       →  mod_stmt stmts $ | stmts $
mod_stmt     →  "module" mod_name
mod_name     →  access | ID
stmts        →  stmts stmt NL | ε
stmt         →  import_stmt | let_stmt | def_stmt | call
import_stmt  →  "import" mod_name
let_stmt     →  "let" assigns
assigns      →  assigns "," assign | assign
assign       →  ID "=" call
def_stmt     →  "def" "(" id_list ")" arg
id_list      →  id_list ID | ID
call         →  call arg | arg
arg          →  lambda
lambda       →  ID "->" lambda | group "->" lambda | logic
logic        →  logic "and" equals | logic "or" equals | equals
equals       →  compare "==" compare | compare "!=" compare | compare
compare      →  member ">" member | member ">=" member | member "<" member | member "<=" member | member
member       →  sum "in" sum | sum
sum          →  sum "+" product | sum "-" product | product
product      →  product "*" unary | product "/" unary | unary
unary        →  "not" primary | primary
primary      →  NUM | ID | STR | literal | symbol | access | group | block | object
literal      →  "true" | "false" | "nil"
symbol       →  ":" ID
access       →  access "." ID | ID "." ID
group        →  "(" call ")"
block        →  do_block | if_block | cond_block
do_block     →  "do" stmts "end"
if_block     →  "if" arg do_else | "if" arg do_block
do_else      →  "do" stmts "else" stmts "end"
cond_block   →  "cond" "do" clauses "end"
clauses      →  clauses clause | ε
clause       →  logic "->" call NL
object       →  list | tuple | map
list         →  "[" items "]"
tuple        →  "#[" items "]"
map          →  "{" entries "}"
items        →  items arg opt_comma | ε
entries      →  entries entry opt_comma | ε
entry        →  ID ":" arg
opt_comma    →  "," | ε
*/

enum {
  ParseSymProgram     =  0,
  ParseSymEOF         =  1,
  ParseSymModule      =  2,
  ParseSymModStmt     =  3,
  ParseSymStmts       =  4,
  ParseSymModName     =  5,
  ParseSymAccess      =  6,
  ParseSymID          =  7,
  ParseSymStmt        =  8,
  ParseSymNL          =  9,
  ParseSymImportStmt  = 10,
  ParseSymLetStmt     = 11,
  ParseSymDefStmt     = 12,
  ParseSymCall        = 13,
  ParseSymImport      = 14,
  ParseSymLet         = 15,
  ParseSymAssigns     = 16,
  ParseSymComma       = 17,
  ParseSymAssign      = 18,
  ParseSymEqual       = 19,
  ParseSymDef         = 20,
  ParseSymLParen      = 21,
  ParseSymIdList      = 22,
  ParseSymRParen      = 23,
  ParseSymArg         = 24,
  ParseSymLambda      = 25,
  ParseSymArrow       = 26,
  ParseSymGroup       = 27,
  ParseSymLogic       = 28,
  ParseSymAnd         = 29,
  ParseSymEquals      = 30,
  ParseSymOr          = 31,
  ParseSymCompare     = 32,
  ParseSymEqualEqual  = 33,
  ParseSymNotEqual    = 34,
  ParseSymMember      = 35,
  ParseSymGreaterThan = 36,
  ParseSymGreaterEqual = 37,
  ParseSymLessThan    = 38,
  ParseSymLessEqual   = 39,
  ParseSymSum         = 40,
  ParseSymIn          = 41,
  ParseSymPlus        = 42,
  ParseSymProduct     = 43,
  ParseSymMinus       = 44,
  ParseSymStar        = 45,
  ParseSymUnary       = 46,
  ParseSymSlash       = 47,
  ParseSymNot         = 48,
  ParseSymPrimary     = 49,
  ParseSymNUM         = 50,
  ParseSymSTR         = 51,
  ParseSymLiteral     = 52,
  ParseSymSymbol      = 53,
  ParseSymBlock       = 54,
  ParseSymObject      = 55,
  ParseSymTrue        = 56,
  ParseSymFalse       = 57,
  ParseSymNil         = 58,
  ParseSymColon       = 59,
  ParseSymDot         = 60,
  ParseSymDoBlock     = 61,
  ParseSymIfBlock     = 62,
  ParseSymCondBlock   = 63,
  ParseSymDo          = 64,
  ParseSymEnd         = 65,
  ParseSymIf          = 66,
  ParseSymDoElse      = 67,
  ParseSymElse        = 68,
  ParseSymCond        = 69,
  ParseSymClauses     = 70,
  ParseSymClause      = 71,
  ParseSymList        = 72,
  ParseSymTuple       = 73,
  ParseSymMap         = 74,
  ParseSymLBracket    = 75,
  ParseSymItems       = 76,
  ParseSymRBracket    = 77,
  ParseSymHashBracket = 78,
  ParseSymLBrace      = 79,
  ParseSymEntries     = 80,
  ParseSymRBrace      = 81,
  ParseSymOptComma    = 82,
  ParseSymEntry       = 83,
  NUM_SYMBOLS
};

i32 GetParseAction(i32 state, u32 sym);
i32 GetParseGoto(i32 state, u32 sym);
i32 GetParseReduction(i32 state);
i32 GetReductionNum(i32 state);
i32 IsLiteral(i32 sym);
char *ParseSymbolName(i32 sym);
