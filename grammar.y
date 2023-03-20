%token id
%token num
%token arrow

%%

expr: lambda;
expr: call;
expr: id;
expr: num;
expr: group;
//expr: list;

lambda: '(' ids ')' arrow expr;
lambda: '(' ')' arrow expr;
call:   '(' ids ')';
call:   '(' ids args ')';
call:   '(' args ')';

ids: ids id;
ids: id;

args: args expr;
args: num;
args: group;
args: call;
args: lambda;

group: '(' sum ')';
group: '(' product ')';

sum: product '-' product;
//sum: product '+' product;
product: negative '*' negative;
//product: negative '/' negative;
negative: '-' expr;
negative: expr;

//list: '[' list_items ']';
//list_items: list_items expr;
//list_items: ;
