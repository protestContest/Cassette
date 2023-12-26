%token ID
%token STR
%token NUM

%%

script: modname imports ws stmts;

modname: "module" ID "\n" | ;
imports: import "\n" | imports "\n" ws import | ;
import: "import" ID | "import" ID "as" ID | "import" ID "as" "*";

stmts: stmt | stmt "\n" ws stmts | ;
stmt: expr | let_stmt | def_stmt;
let_stmt: "let" ws assigns;
assigns: assign | assigns "," ws assign;
assign: ID ws "=" ws expr;
def_stmt: "def" "(" ws params ws ")" expr;

expr: lambda | call;
call: access "(" ws args ws ")" | access "(" ws ")";
args: expr | args "," ws expr;

lambda: logic | "\\" ws params ws "->" ws expr | "\\" ws "->" ws expr;
params: ID | params "," ws ID;
logic: equality | logic "and" ws equality | logic "or" ws equality;
equality: compare | equality "==" ws compare | equality "!=" ws compare;
compare: member | compare "<" ws member | compare "<=" ws member | compare ">" ws member | compare ">=" ws member;
member: concat | member "in" ws concat;
concat: pair | concat "<>" ws pair;
pair: bitwise | bitwise "|" ws pair;
bitwise: shift | bitwise "&" ws shift | bitwise "^" ws shift;
shift: sum | shift ">>" ws sum | shift "<<" ws sum;
sum: product | product "+" ws sum | product "-" ws sum;
product: unary | product "*" ws unary | product "/" ws unary | product "%" ws unary;
unary: access | "-" access | "#" access | "not" access;
access: primary | access "." primary;

primary: group | block | obj | value | ID;
group: "(" ws expr ws ")";

block: do | if;
do: "do" ws stmts "end";
if: "if" ws expr "do" ws stmts "else" ws stmts "end" | "if" ws expr "do" ws stmts "end";

obj: list | tuple | map;
list: "[" ws items ws "]" | "[" ws "]";
tuple: "{" ws items ws "}" | "{" ws "}";
map: "{" ws entries ws "}" | "{:" ws "}";
items: expr | items "," ws expr;
entries: entry | entries "," ws entry;
entry: ID ":" ws expr;

value: "true" | "false" | "nil" | symbol | NUM | STR;
symbol: ":" ID;

ws: " " ws | "\n" ws | "\t" ws | ;
