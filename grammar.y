%token id
%token num

%%

expr: product;
expr: ;

product: product '*' sum;
product: product '/' sum;
product: sum;

sum: sum '+' primary;
sum: sum '-' primary;
sum: primary;

primary: value;
primary: '(' expr ')';
value: id;
value: num;
