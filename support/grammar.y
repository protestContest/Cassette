%token ID
%token STR
%token NUM

%%

script: modname imports nl stmts;

modname: "module" ID "\n" | ;
imports: import "\n" | imports "\n" nl import | ;
import: "import" ID | "import" ID "as" ID | "import" ID "as" "*";

stmts: stmt | stmt "\n" nl stmts | ;
stmt: expr | let_stmt | def_stmt;
let_stmt: "let" nl assigns;
assigns: assign | assigns "," nl assign;
assign: ID nl "=" nl expr;
def_stmt: "def" ID "(" nl params nl ")" nl expr
        | "def" ID "(" nl ")" nl expr
        | "def" ID expr;

expr: lambda;

lambda: logic | "\\" nl params nl "->" nl expr | "\\" nl "->" nl expr;
params: ID | params "," nl ID;
logic: equality | logic "and" nl equality | logic "or" nl equality;
equality: compare | equality "==" nl compare | equality "!=" nl compare;
compare: member | compare "<" nl member | compare "<=" nl member | compare ">" nl member | compare ">=" nl member;
member: concat | member "in" nl concat;
concat: pair | concat "<>" nl pair;
pair: bitwise | bitwise "|" nl pair;
bitwise: shift | bitwise "&" nl shift | bitwise "^" nl shift;
shift: sum | shift ">>" nl sum | shift "<<" nl sum;
sum: product | product "+" nl sum | product "-" nl sum;
product: unary | product "*" nl unary | product "/" nl unary | product "%" nl unary;
unary: call | "-" call | "#" call | "~" call | "not" call;
call: access "(" nl args nl ")" | access "(" nl ")";
args: expr | args "," nl expr;
access: primary | access "." primary;

primary: group | block | obj | value | ID;
group: "(" nl expr nl ")";

block: do | if | cond;
do: "do" nl stmts "end";
if: "if" nl expr "do" nl stmts "else" nl stmts "end" | "if" nl expr "do" nl stmts "end";
cond: "cond" "do" clauses "end";
clauses: clauses "\n" nl clause | ;
clause: expr nl "->" nl expr;

obj: list | tuple | map;
list: "[" nl items nl "]" | "[" nl "]";
tuple: "{" nl items nl "}" | "{" nl "}";
map: "{" nl entries nl "}" | "{:" nl "}";
items: expr | items "," nl expr;
entries: entry | entries "," nl entry;
entry: ID ":" nl expr;

value: "true" | "false" | "nil" | symbol | NUM | STR;
symbol: ":" ID;

nl: "\n" nl | ;
