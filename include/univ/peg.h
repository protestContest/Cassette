#pragma once
#include "univ/hashmap.h"

typedef struct {
  char start;
  char end;
} PRange;

typedef enum {pegChoice, pegSeq, pegRule, pegClass, pegLiteral, pegAny} PExprType;

typedef struct PExpr {
  PExprType type;
  enum {pegMatch, pegAssert, pegRefute} predicate;
  enum {pegOne, pegOptional, pegZeroPlus, pegOnePlus} quantity;
  union {
    struct PExpr **parts;
    PRange *ranges;
    char *text;
  } data;
} PExpr;

typedef struct PNode {
  char *name;
  char *lexeme;
  i32 length;
  struct PNode **elements; /* vec */
  union {
    i32 num;
    void *data;
  } value;
} PNode;

typedef PNode* (*PRuleAction)(PNode *node);

typedef struct {
  char *name;
  PExpr *expr;
  PRuleAction action;
} PRule;

typedef struct {
  HashMap map;
  PRule **rules; /* vec */
} Grammar;

Grammar *NewGrammar(void);
void FreeGrammar(Grammar *g);
void AddRule(char *name, char *expr, PRuleAction action, Grammar *g);
Grammar *ReadGrammar(char *text);
void OnRule(char *name, PRuleAction action, Grammar *g);

PNode *TransparentNode(PNode *node);
PNode *TransparentChild(PNode *node);
PNode *DiscardNode(PNode *node);
PNode *TextNode(PNode *node);

PNode *ParseRule(char *name, char *text, u32 *index, u32 length, Grammar *g);
void FreePNode(PNode *node);

#ifdef DEBUG
void PrintPExpr(PExpr *expr);
void PrintGrammar(Grammar *g);
void PrintPNode(PNode *node);
#endif
