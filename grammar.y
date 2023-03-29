%token ID
%token NUM
%token STR
%token NEWLINE
%token COND
%token DEF
%token DO
%token ELSE
%token END
%token IF
%token ARROW
%token DCOLON
%token LET

%%

expr: do_block;
expr: if_block;
expr: cond_block;
expr: define;
expr: let;
expr: call;

do_block: DO block END;

if_block: IF arg DO block END;
if_block: IF arg DO block ELSE block END;
if_block: IF arg arg;

cond_block: COND DO clauses END;
clauses: clauses NEWLINE expr DCOLON expr;
clauses: ;

define: DEF ID expr;
define: DEF '(' params ')' expr;
params: params ID;
params: ;

let: LET dict expr;

block: block NEWLINE expr;
block: expr;

call: call arg;
call: arg;
arg: equal;
arg: entry;

equal: equal '=' pair;
equal: pair;
pair: compare '|' compare;
pair: compare;
compare: sum '<' sum;
compare: sum '>' sum;
compare: sum;
sum: sum '+' product;
sum: sum '-' product;
sum: product;
product: product '*' primary;
product: product '/' primary;
product: primary;

primary: ID;
primary: NUM;
primary: STR;
primary: group;
primary: lambda;
primary: list;
primary: dict;

group: '(' expr ')';
lambda: group ARROW primary;

list: '[' items ']';
items: items primary;
items: primary;

dict: '{' entries '}';
entries: entries entry;
entries: ;
entry: ID ':' equal;
