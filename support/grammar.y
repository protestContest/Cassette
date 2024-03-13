%token ID
%token STR
%token NUM

%%

module:     modname imports stmts;
modname:    "module" ID | ;
imports:    import imports | ;
import:     "import" ID | "import" ID "as" ID;

stmts:      stmt stmts | ;
stmt:       let_stmt | def_stmt | expr;
let_stmt:   "let" assigns;
assigns:    assign | assigns "," assign;
assign:     ID "=" expr;
def_stmt:   "def" ID "(" params ")" expr;
params:     ID | ID "," params | ;

expr:       lambda;
lambda:     pair | "\\" params "->" lambda;
logic:      equality | logic "and" equality | logic "or" equality;
equality:   member | equality "==" member;
pair:       logic | logic "|" pair;
join:       split | join "<>" split;
split:      compare | split ":" compare | split "@" compare;
compare:    shift | compare "<" shift | compare ">" shift;
shift:      sum | shift ">>" sum | shift "<<" sum;
sum:        product | sum "+" product | sum "-" product | sum "^" product;
product:    unary | product "*" unary | product "/" unary | product "%" unary | product "&" unary;
unary:      call | "-" call | "~" call | "#" call | "not" call;
call:       access | call "(" params ")";
access:     primary | access "." primary;
primary:    group | block | object | value;

group:      "(" expr ")";

block:      do_block | if_block | cond_block;
do_block:   "do" stmts "end";
if_block:   "if" expr "do" stmts "else" stmts "end";
cond_block: "cond" clauses "end";
clauses:    clause clauses | ;
clause:     expr "->" expr;

object:     list | tuple;
list:       "[" list_tail;
list_tail:  "]" | expr "]" | expr "," list_tail;
tuple:      "{" tuple_tail;
tuple_tail: "}" | expr "}" | expr "," tuple_tail;

value:      NUM | ID | STR | "true" | "false" | "nil";
