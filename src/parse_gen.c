#include "parse_gen.h"
#include "compile/lex.h"
#include <univ/base.h>
#include <univ/io.h>
#include <univ/math.h>
#include <univ/vec.h>
#include <univ/hash.h>
#include <stdio.h>

typedef struct {
  u32 id;
  Token token;
} Symbol;

typedef struct {
  u32 left;
  u32 *right;
} Production;

typedef struct {
  Production *productions;
  Symbol *symbols;
  u32 lhs_size;
} Grammar;

typedef struct {
  u32 production;
  u32 position;
} Config;

typedef struct {
  u32 symbol;
  Config *items;
  u32 next_state;
} StateSymbol;

typedef struct {
  StateSymbol *symbols;
} ParseState;

typedef enum {
  YaccEOF,
  YaccDefToken,
  YaccSep,
  YaccSymbol,
  YaccTerminal,
  YaccColon,
  YaccSemicolon,
} YaccTokenType;

static Token YaccToken(Lexer *lex)
{
  if (LexPeek(lex, 0) == '\0') return MakeToken(YaccEOF, lex);

  if (Match(lex, "%token")) return MakeToken(YaccDefToken, lex);
  if (Match(lex, "%%")) return MakeToken(YaccSep, lex);
  if (Match(lex, ":")) return MakeToken(YaccColon, lex);
  if (Match(lex, ";")) return MakeToken(YaccSemicolon, lex);

  if (Match(lex, "'")) {
    lex->start = lex->pos;
    while (LexPeek(lex, 0) != '\'') lex->pos++;
    Token token = MakeToken(YaccTerminal, lex);
    lex->pos++;
    return token;
  }

  if (IsAlpha(LexPeek(lex, 0))) {
    while (IsAlpha(LexPeek(lex, 0)) || LexPeek(lex, 0) == '_') lex->pos++;
    return MakeToken(YaccSymbol, lex);
  }

  return ErrorToken(lex, "Invalid token");
}

#define NullSymbol        ((Symbol){0, {YaccEOF, 0, 0, NULL, 0}})
#define IsNullSymbol(sym) ((sym).token.type == YaccEOF)

static u32 PutSymbol(Symbol *syms, Token token)
{
  u32 id = Hash(token.lexeme, token.length);
  u32 i = id % VecCapacity(syms);

  while (!IsNullSymbol(syms[i]) && syms[i].id != id) {
    i = (i + 1) % VecCapacity(syms);
  }

  if (IsNullSymbol(syms[i])) {
    syms[i] = (Symbol){id, token};
  }

  return id;
}

static Symbol GetSymbol(Symbol *syms, u32 id)
{
  u32 i = id % VecCapacity(syms);
  while (syms[i].id != id) {
    i = (i + 1) % VecCapacity(syms);
  }
  return syms[i];
}

static Grammar ParseGrammar(char *grammar_file)
{
  char *src = ReadFile(grammar_file);

  Grammar g = {NULL, NewVec(Symbol, 256), 4};

  for (u32 i = 0; i < VecCapacity(g.symbols); i++) {
    g.symbols[i] = NullSymbol;
  }

  u32 end_sym = PutSymbol(g.symbols, (Token){YaccTerminal, 0, 0, "$", 1});

  Lexer lex;
  InitLexer(&lex, YaccToken, src);
  Token token = NextToken(&lex);

  while (token.type != YaccEOF && !IsErrorToken(token) && token.type != YaccSep) {
    if (token.type != YaccDefToken) {
      printf("Expected '%%token' at %s:%d:%d\n", grammar_file, lex.line, lex.col);
      PrintSourceContext(&lex, 3);
      return g;
    }

    token = NextToken(&lex);
    if (token.type != YaccSymbol) {
      printf("Error at %s:%d:%d\n", grammar_file, lex.line, lex.col);
      return g;
    }

    token.type = YaccTerminal;
    PutSymbol(g.symbols, token);
    token = NextToken(&lex);
  }

  if (token.type != YaccSep) {
    printf("No rules!\n");
    return g;
  }

  Production top_prod;
  top_prod.left = PutSymbol(g.symbols, (Token){YaccSymbol, 0, 0, "S", 1});
  top_prod.right = NULL;
  VecPush(g.productions, top_prod);

  token = NextToken(&lex);
  while (token.type == YaccSymbol) {
    Production *p = VecNext(g.productions);
    p->left = PutSymbol(g.symbols, token);

    if (token.length + 2 > g.lhs_size) g.lhs_size = token.length + 2;

    token = NextToken(&lex);

    if (token.type != YaccColon) {
      printf("Expected colon at %s:%d:%d\n", grammar_file, lex.line, lex.col);
      return g;
    }

    token = NextToken(&lex);
    while (token.type == YaccSymbol || token.type == YaccTerminal) {
      u32 id = PutSymbol(g.symbols, token);
      VecPush(p->right, id);
      token = NextToken(&lex);
    }

    if (token.type == YaccSemicolon) token = NextToken(&lex);
  }

  if (VecCount(g.productions) > 1) {
    VecPush(g.productions[0].right, g.productions[1].left);
  }
  VecPush(g.productions[0].right, end_sym);

  if (IsErrorToken(token)) {
    printf("Error at %s:%d:%d\n", grammar_file, lex.line, lex.col);
    return g;
  }

  return g;
}

static void PrintGrammar(Grammar g)
{
  printf("Terminals: ");
  for (u32 i = 0; i < VecCapacity(g.symbols); i++) {
    if (g.symbols[i].token.type == YaccTerminal) {
      PrintToken(g.symbols[i].token);
      printf(" ");
    }
  }
  printf("\n\n");

  printf("Rules:\n");
  u32 last_lhs = 0;
  for (u32 i = 0; i < VecCount(g.productions); i++) {
    printf("%4d  ", i);
    Production p = g.productions[i];

    if (p.left == last_lhs) {
      for (u32 i = 0; i < g.lhs_size; i++) printf(" ");
    } else {
      last_lhs = p.left;
      Token lhs = GetSymbol(g.symbols, p.left).token;
      PrintToken(lhs);
      for (u32 i = 0; i < g.lhs_size - lhs.length; i++) printf(" ");
    }

    printf("→ ");

    if (VecCount(p.right) == 0) {
      printf("ε");
    } else {
      for (u32 j = 0; j < VecCount(p.right); j++) {
        Token t = GetSymbol(g.symbols, p.right[j]).token;
        PrintToken(t);
        printf(" ");
      }
    }
    printf("\n");
  }
}

static void PrintConfig(Grammar g, Config c)
{
  printf("%4d  ", c.production);
  Token token = GetSymbol(g.symbols, g.productions[c.production].left).token;
  PrintToken(token);
  for (u32 i = 0; i < g.lhs_size - token.length; i++) printf(" ");
  printf("→ ");

  for (u32 i = 0; i < VecCount(g.productions[c.production].right); i++) {
    if (i == c.position) printf("· ");
    token = GetSymbol(g.symbols, g.productions[c.production].right[i]).token;
    PrintToken(token);
    printf(" ");
  }
  if (c.position == VecCount(g.productions[c.production].right)) {
    printf("·");
  }
}

static Symbol NextSymbol(Grammar g, Config item)
{
  Production p = g.productions[item.production];
  if (item.position < VecCount(p.right)) {
    return GetSymbol(g.symbols, p.right[item.position]);
  } else {
    return NullSymbol;
  }
}

// static void PrintConfigSet(Grammar g, ConfigSet set, u32 indent)
// {
//   for (u32 i = 0; i < indent; i++) printf("  ");
//   PrintConfig(g, set.items[0]);
//   printf("\n");

//   // Symbol next = NextSymbol(g, set, set.items[0]);
//   for (u32 i = 1; i < VecCount(set.items); i++) {
//     // Production p = g.productions[set.items[i].production];
//     // if (p.left == next.id || true) {
//       for (u32 j = 0; j < indent; j++) printf("  ");
//       PrintConfig(g, set.items[i]);
//       printf("\n");
//     // }
//   }
// }

// static bool ConfigSetHasConfig(ConfigSet set, Config item)
// {
//   for (u32 i = 0; i < VecCount(set.items); i++) {
//     if (set.items[i].production == item.production && set.items[i].position == item.position) {
//       return true;
//     }
//   }

//   return false;
// }

static void Closure(Grammar g, ParseState *state)
{
  for (u32 i = 0; i < VecCount(state->symbols); i++) {

  }
}

// static u32 Successor(ConfigSet *family, u32 id)
// {
//   for (u32 i = 0; i < VecCount(family); i++) {
//     if (family[i].id == id) {
//       return i;
//     }
//   }

//   return 0;
// }

// static void PrintFamily(Grammar g, ConfigSet *family)
// {
//   bool print_all = true;

//   for (u32 i = 0; i < VecCount(family); i++) {
//     printf("State %d (%d)\n", i, VecCount(family[i].items));

//     printf("  ");
//     PrintConfig(g, family[i].items[0]);
//     u32 succ = Successor(family, family[i].items[0].successor);
//     if (succ == 0) {
//       printf("    Reduce %d\n", family[i].items[0].production);
//     } else {
//       printf("    Successor: %d\n", succ);
//     }

//     Symbol next = NextSymbol(g, family[i].items[0]);
//     for (u32 j = 1; j < VecCount(family[i].items); j++) {
//       Production p = g.productions[family[i].items[j].production];
//       if ((p.left == next.id) || print_all) {
//         printf("  ");
//         PrintConfig(g, family[i].items[j]);
//         printf("    Successor: %d\n", Successor(family, family[i].items[j].successor));
//       }
//     }


//   }
// }

// static bool FamilyHasSet(ConfigSet *family, ConfigSet set)
// {
//   for (u32 i = 0; i < VecCount(family); i++) {
//     if (family[i].id == set.id) return true;
//   }
//   return false;
// }

// static bool SymbolSeen(Symbol sym, u32 *seen)
// {
//   for (u32 i = 0; i < VecCount(seen); i++) {
//     if (seen[i] == sym.id) return true;
//   }
//   return false;
// }

// static ConfigSet NextSet(ConfigSet *family, Grammar g, ConfigSet set, Symbol sym)
// {
//   ConfigSet next_set = {EmptyHash(), NULL};

//   for (u32 k = 0; k < VecCount(set.items); k++) {
//     if (NextSymbol(g, set.items[k]).id == sym.id) {
//       Config item = {set.items[k].production, set.items[k].position + 1, 0};
//       VecPush(next_set.items, item);
//       next_set.id = AddHash(next_set.id, &item, sizeof(Config));
//     }
//   }

//   return next_set;
// }

// static ConfigSet *BuildFamilyRest(ConfigSet *family, Grammar g, ConfigSet set)
// {
//   VecPush(family, set);

//   u32 *seen = NULL;
//   for (u32 i = 0; i < VecCount(set.items); i++) {
//     Symbol sym = NextSymbol(g, set.items[i]);
//     if (IsNullSymbol(sym)) continue;
//     if (SymbolSeen(sym, seen)) continue;
//     VecPush(seen, sym.id);

//     ConfigSet next_set = NextSet(family, g, set, sym);

//     if (!FamilyHasSet(family, next_set)) {
//       for (u32 k = i; k < VecCount(set.items); k++) {
//         if (NextSymbol(g, set.items[k]).id == sym.id) {
//           set.items[k].successor = next_set.id;
//         }
//       }

//       family = BuildFamilyRest(family, g, next_set);
//     }
//   }
//   FreeVec(seen);

//   return family;
// }

// static ConfigSet *BuildFamily(Grammar g)
// {
//   Config initial_item = (Config){0, 0, 0};
//   ConfigSet initial_set = {0, NULL};
//   VecPush(initial_set.items, initial_item);
//   initial_set = Closure(g, initial_set);

//   // ConfigSet *family = NULL;
//   // VecPush(family, Closure(g, initial_set));

//   return BuildFamilyRest(NULL, g, initial_set);
// }

void Generate(char *grammar)
{
  Grammar g = ParseGrammar(grammar);
  PrintGrammar(g);
  printf("\n");

  ConfigSet *family = BuildFamily(g);
  PrintFamily(g, family);
}
