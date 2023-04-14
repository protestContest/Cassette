#pragma once

/*
Grammar:
  1 program         → block $
  2 block           → block stmt
  3 block           → newlines
  4 block           → ε
  5 stmt            → expr newlines
  6 expr            → "def" ID sum
  7 expr            → call
  8 call            → call sum
  9 call            → sum
 10 sum             → sum "+" product
 11 sum             → sum "-" product
 12 sum             → product
 13 product         → product "*" primary
 14 product         → product "/" primary
 15 product         → primary
 16 primary         → NUM
 17 primary         → ID
 18 primary         → group
 19 primary         → lambda
 20 primary         → do_block
 21 do_block        → "do" block "end"
 22 group           → "(" expr ")"
 23 lambda          → group "->" primary
 24 newlines        → NL
 25 newlines        → $
*/

char *symbol_names[] = {
  [0] = "$",
  [1] = "def",
  [2] = "ID",
  [3] = "+",
  [4] = "-",
  [5] = "*",
  [6] = "/",
  [7] = "NUM",
  [8] = "do",
  [9] = "end",
  [10] = "(",
  [11] = ")",
  [12] = "->",
  [13] = "NL",
  [14] = "program",
  [15] = "block",
  [16] = "stmt",
  [17] = "expr",
  [18] = "call",
  [19] = "sum",
  [20] = "product",
  [21] = "primary",
  [22] = "do_block",
  [23] = "group",
  [24] = "lambda",
  [25] = "newlines",
};
#define TOP_SYMBOL 14

// indexed by state
static unused i32 reduction_syms[] = {
  15,  -1,  15,  25,  25,  15,  14,  -1,  -1,  17,  18,  19,  20,  21,  21,  21,  21,  21,  -1,  15,
  16,  -1,  18,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  17,  19,  19,  20,  20,  24,  23,  22,
};

// indexed by state
static unused u32 reduction_sizes[] = {
   0,   0,   1,   1,   1,   2,   2,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,
   2,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   3,   3,   3,   3,   3,   3,   3,   3,
};

// indexed by state, symbol
#define NUM_STATES 39
#define NUM_SYMS 26
static unused i32 actions[][NUM_SYMS] = {
  {   4,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   3,
     -1,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   2,},
  {   6,   8,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,   5,   7,   9,  10,  11,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {   4,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   3,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  20,},
  {  -1,  -1,  21,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  22,  11,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  -1,  24,  23,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  26,  25,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  27,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,   8,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  29,  -1,  -1,  -1,
     -1,  -1,  -1,  28,   9,  10,  11,  12,  17,  15,  16,  -1,},
  {   4,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   3,
     -1,  30,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   2,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  31,  11,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  -1,  24,  23,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  32,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  33,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  34,  17,  15,  16,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  35,  17,  15,  16,  -1,},
  {  -1,  -1,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  36,  17,  15,  16,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  37,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,   8,  14,  -1,  -1,  -1,  -1,  13,  19,  -1,  18,  -1,  -1,  -1,
     -1,  -1,  -1,  28,   9,  10,  11,  12,  17,  15,  16,  -1,},
  {  -1,   8,  14,  -1,  -1,  -1,  -1,  13,  19,  38,  29,  -1,  -1,  -1,
     -1,  -1,   5,   7,   9,  10,  11,  12,  17,  15,  16,  -1,},
  {  -1,  -1,  -1,  24,  23,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  26,  25,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  26,  25,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
  {  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,},
};
