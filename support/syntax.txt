%token ID
%token NUM
%token STR
%token sp

%%

Module:     vsp ModName Imports Exports Stmts ;
ModName:    "module" sp ID NL vsp | ;
Imports:    "import" ImportList NL vsp | ;
ImportList: vsp Import | ImportList "," vsp Import ;
Import:     ID sp Alias sp FnList ;
Alias:      "as" sp ID | ;
FnList:     "(" IDList ")" | ;
Exports:    "export" vsp IDList NL vsp | ;
Stmts:      vsp Stmt | Stmts NL vsp Stmt ;
Stmt:       Def | Record | Expr ;
Def:        "def" sp ID "(" vsp Params ")" vsp DefGuard vsp Expr ;
Params:     IDList | ;
DefGuard:   "when" vsp Expr sp cnl | ;
Record:     "record" sp ID "(" vsp Params ")" ;
Expr:       Lambda sp ;
Lambda:     "\\" sp Params "->" vsp Expr | Logic ;
Logic:      Equal | Logic "and" vsp Equal | Logic "or" vsp Equal ;
Equal:      Pair | Pair "==" vsp Pair | Pair "!=" vsp Pair ;
Pair:       Join | Join  ":" vsp Pair ;
Join:       Compare | Join "<>" vsp Compare ;
Compare:    Shift | Compare "<" vsp Shift | Compare ">" vsp Shift ;
Shift:      Sum | Shift "<<" vsp Sum | Shift ">>" vsp Sum ;
Sum:        Product | Sum SumOp vsp Product ;
SumOp:      "+" | "-" | "|" | "^";
Product:    Unary | Product ProdOp vsp Unary ;
ProdOp:     "*" | "/" | "%" | "&" ;
Unary:      UnOp Call | "not" vsp Call | Call ;
UnOp:       "-" | "~" | "#" | "@" | "^" ;
Call:       Qualifier | Call Args | Call Slice ;
Args:       "(" vsp ArgsTail ;
ArgsTail:   ")" | Items ")" ;
Slice:      "[" vsp Expr vsp SliceEnd "]" ;
SliceEnd:   "," vsp Expr vsp | ;
Qualifier:  Primary | ID "." ID ;
Primary:    Group | Do | Let | If | List | Tuple | Const | ID ;
Group:      "(" vsp Expr vsp ")" ;
Do:         "do" vsp Stmts "end" ;
Let:        "let" Assigns vsp "in" vsp Expr;
Assigns:    Assign | Assigns NL vsp Assign ;
Assign:     ID sp "=" vsp Expr LetGuard ;
LetGuard:   "except" vsp Expr sp cnl vsp Expr | ;
If:         "if" vsp Clauses vsp "else" vsp Expr ;
Clauses:    Clause | Clauses NL vsp Clause ;
Clause:     Expr sp "," vsp Expr ;
List:       "[" vsp ListTail ;
ListTail:   "]" | Items "]" ;
Tuple:      "{" vsp TupleTail ;
TupleTail:  "}" | Items "}" ;
Const:      NUM | STR | ":" ID | Literal ;
Literal:    "nil" | "true" | "false" ;
Items:      Expr vsp | Items "," vsp Expr vsp ;
IDList:     ID sp | IDList "," vsp ID sp ;
vsp:        sp NL vsp | sp ;
cnl:        "," | NL ;
NL:         "\n" ;
