%%

L: L ';' E;
L: E;
E: E ',' P;
E: P;
P: 'a';
P: '(' M ')';
M: L;
M: ;
