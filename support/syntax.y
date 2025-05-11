Module    <- vsp ModName? Imports? Stmts EOF
ModName   <- "module" sp ID nl vsp
Imports   <- "import" vsp Import ("," vsp Import)* nl vsp
Import    <- ID sp Alias? FnList?
Alias     <- "as" sp ID sp
FnList    <- "(" vsp Params vsp ")" sp
Stmts     <- vsp Stmt (nl vsp Stmt)* vsp
Stmt      <- Def / Let / Guard / Record / Expr
Def       <- "def" sp ID "(" vsp Params? vsp ")" vsp DefGuard? Expr
DefGuard  <- "when" vsp Expr sp ("," / nl) vsp
Let       <- "let" Assigns
Assigns   <- vsp Assign ("," vsp Assign)*
Assign    <- ID sp "=" vsp Expr
Guard     <- "guard" vsp Expr vsp "else" vsp Expr
Record    <- "record" sp ID "(" vsp Params vsp ")"
Expr      <- Lambda
Lambda    <- "\\" vsp Params? vsp "->" vsp Expr / Logic
Params    <- ID sp ("," vsp ID sp)*
Logic     <- Equal (("and" / "or") vsp Equal)*
Equal     <- Pair (("==" / "!=") vsp Pair)*
Pair      <- Join (":" vsp Join)*
Join      <- Compare ("<>" vsp Compare)*
Compare   <- Shift (("<" / ">") sp Shift)*
Shift     <- Sum (("<<" / ">>") sp Sum)*
Sum       <- Product (("+" / "-" / "|" / "^") sp Product)*
Product   <- Unary (("*" / "/" / "%" / "&") sp Unary)*
Unary     <- "not" vsp Call / [-~#@^]? Call
Call      <- Qualifier (Args / Slice)?
Args      <- "(" vsp Items? vsp ")"
Slice     <- "[" vsp Expr vsp ("," vsp Expr vsp)? "]"
Qualifier <- ID "." ID sp / Primary
Primary   <- (Group / List / Tuple / Do / If / Const / ID) sp
Const     <- NUM / STR / ":" ID / Literal
Literal   <- "nil" / "true" / "false"
Group     <- "(" vsp Expr vsp ")"
List      <- "[" vsp Items? vsp "]"
Tuple     <- "{" vsp Items? vsp "}"
Items     <- Expr ("," vsp Expr)*
Do        <- "do" Stmts "end"
If        <- "if" Clauses "else" vsp Expr
Clauses   <- vsp Clause (nl vsp Clause)* vsp
Clause    <- Expr sp "," vsp Expr

ID        <- !Reserved [A-Za-z_][A-Za-z0-9_]*
Reserved  <- ("def" / "let" / "guard" / "record" /
              "not" / "if" / "else" / "do" / "end") ![A-Za-z0-9_]
NUM       <- "0x"? [0-9_]+
STR       <- '"' ((!'"' .) / '\\"')* '"'
vsp       <- (' ' / '\t' / nl / Comment)*
sp        <- (' ' / '\t' / Comment)*
Comment   <- ';' (!nl .)* nl
nl        <- '\n' / '\r'
EOF       <- !.
