%token ID

%%

E: E '+' T;
E: T;
T: '(' E ')';
T: ID;
