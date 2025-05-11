#include "univ/peg.h"
#include "univ/vec.h"
#include "univ/math.h"
#include "univ/str.h"

Grammar *NewGrammar(void)
{
  Grammar *g = malloc(sizeof(Grammar));
  InitHashMap(&g->map);
  g->rules = 0;
  return g;
}

static void FreePRule(PRule *rule);

void FreeGrammar(Grammar *g)
{
  u32 i;
  for (i = 0; i < VecCount(g->rules); i++) {
    FreePRule(g->rules[i]);
  }
  DestroyHashMap(&g->map);
  FreeVec(g->rules);
  free(g);
}

static PExpr *NewPExpr(PExprType type)
{
  PExpr *expr = malloc(sizeof(PExpr));
  expr->type = type;
  expr->predicate = pegMatch;
  expr->quantity = pegOne;
  expr->data.parts = 0;
  return expr;
}

static PExpr *ClonePExpr(PExpr *expr)
{
  PExpr *clone = NewPExpr(expr->type);
  u32 i;
  switch (expr->type) {
  case pegChoice:
  case pegSeq:
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      VecPush(clone->data.parts, ClonePExpr(expr->data.parts[i]));
    }
    break;
  case pegClass:
    for (i = 0; i < VecCount(expr->data.ranges); i++) {
      VecPush(clone->data.ranges, expr->data.ranges[i]);
    }
    break;
  case pegRule:
  case pegLiteral:
    clone->data.text = malloc(StrLen(expr->data.text)+1);
    Copy(expr->data.text, clone->data.text, StrLen(expr->data.text)+1);
    break;
  case pegAny:
    break;
  }
  return clone;
}

static void FreePExpr(PExpr *expr)
{
  u32 i;
  switch (expr->type) {
  case pegChoice:
  case pegSeq:
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      FreePExpr(expr->data.parts[i]);
    }
    FreeVec(expr->data.parts);
    break;
  case pegClass:
    FreeVec(expr->data.ranges);
    break;
  case pegRule:
  case pegLiteral:
    free(expr->data.text);
    break;
  case pegAny:
    break;
  }
  free(expr);
}

static PNode *NewPNode(char *name)
{
  PNode *node = malloc(sizeof(PNode));
  node->name = NewString(name);
  node->lexeme = 0;
  node->length = 0;
  node->elements = 0;
  node->value.data = 0;
  return node;
}

void FreePNode(PNode *node)
{
  u32 i;
  if (!node) return;
  if (node->name) free(node->name);
  for (i = 0; i < VecCount(node->elements); i++) {
    FreePNode(node->elements[i]);
  }
  FreeVec(node->elements);
  free(node);
}

static void TruncNodes(PNode **nodes, u32 index)
{
  u32 i;
  for (i = index; i < VecCount(nodes); i++) {
    FreePNode(nodes[i]);
  }
  VecTrunc(nodes, index);
}

#define Fail(free, expr) do {free(expr); return 0;} while (0)

static void SkipSpacing(char *str, u32 *index)
{
  while (str[*index] && IsWhitespace(str[*index])) (*index)++;
  if (str[*index] && str[*index] == '#') {
    while (str[*index] && !IsNewline(str[*index])) (*index)++;
    SkipSpacing(str, index);
  }
}

static PExpr *ParseChoice(char *str, u32 *index);

static bool ParseChar(char *str, u32 *index, char *ch)
{
  if (!str[*index]) return false;
  if (str[*index] == '\\') {
    (*index)++;
    if (!str[*index]) return false;

    if (IsHexDigit(str[*index]) && IsHexDigit(str[*index + 1])) {
      *ch = HexByte(str[*index])*16 + HexByte(str[*index+1]);
      (*index) += 2;
    } else {
      switch (str[*index]) {
      case 'r':
        *ch = '\r';
        break;
      case 'n':
        *ch = '\n';
        break;
      case 't':
        *ch = '\t';
        break;
      case '\'':
      case '"':
      case '[':
      case ']':
      case '\\':
        *ch = str[*index];
        break;
      default:
        return false;
      }
      (*index)++;
    }
  } else {
    *ch = str[*index];
    (*index)++;
  }

  return true;
}

static PExpr *ParseLiteral(char *str, u32 *index)
{
  PExpr *expr;
  char quote = str[*index];
  char *text = 0;
  (*index)++;
  while (str[*index] && str[*index] != quote) {
    char ch;
    if (!ParseChar(str, index, &ch)) Fail(FreeVec, text);
    VecPush(text, ch);
  }

  if (!str[*index]) Fail(FreeVec, text);
  if (str[*index] != quote) Fail(FreeVec, text);
  (*index)++;
  SkipSpacing(str, index);

  expr = NewPExpr(pegLiteral);
  expr->data.text = malloc(VecCount(text) + 1);
  Copy(text, expr->data.text, VecCount(text));
  expr->data.text[VecCount(text)] = 0;
  FreeVec(text);

  return expr;
}

static PExpr *ParseClass(char *str, u32 *index)
{
  PExpr *expr = NewPExpr(pegClass);
  (*index)++;
  while (str[*index] && str[*index] != ']') {
    PRange range;
    if (!ParseChar(str, index, &range.start)) Fail(FreePExpr, expr);
    if (str[*index] == '-') {
      (*index)++;
      if (!ParseChar(str, index, &range.end)) Fail(FreePExpr, expr);
    } else {
      range.end = range.start;
    }
    VecPush(expr->data.ranges, range);
  }

  if (str[*index] != ']') Fail(FreePExpr, expr);
  (*index)++;
  SkipSpacing(str, index);

  return expr;
}

#define IsIDChar(c) (IsAlpha(c) || IsDigit(c) || (c) == '_')

static PExpr *ParseIdentifier(char *str, u32 *index)
{
  PExpr *expr = NewPExpr(pegRule);
  i32 start = *index;
  while (IsIDChar(str[*index])) (*index)++;
  expr->data.text = malloc(*index - start + 1);
  Copy(str + start, expr->data.text, *index - start);
  expr->data.text[*index-start] = 0;
  SkipSpacing(str, index);
  return expr;
}

static PExpr *ParsePrimary(char *str, u32 *index)
{
  PExpr *expr;

  if (IsAlpha(str[*index]) || str[*index] == '_') {
    expr = ParseIdentifier(str, index);
    if (!expr) return expr;
    if (Match("<-", str + *index)) Fail(FreePExpr, expr);
  } else {
    switch (str[*index]) {
    case '(':
      (*index)++;
      SkipSpacing(str, index);
      expr = ParseChoice(str, index);
      if (!expr) return expr;
      if (str[*index] != ')') Fail(FreePExpr, expr);
      (*index)++;
      SkipSpacing(str, index);
      break;
    case '"':
    case '\'':
      expr = ParseLiteral(str, index);
      break;
    case '[':
      expr = ParseClass(str, index);
      break;
    case '.':
      (*index)++;
      SkipSpacing(str, index);
      expr = NewPExpr(pegAny);
      break;
    default:
      return 0;
    }
  }

  if (str[*index] == '?') {
    (*index)++;
    SkipSpacing(str, index);
    expr->quantity = pegOptional;
  } else if (str[*index] == '*') {
    (*index)++;
    SkipSpacing(str, index);
    expr->quantity = pegZeroPlus;
  } else if (str[*index] == '+') {
    (*index)++;
    SkipSpacing(str, index);
    expr->quantity = pegOnePlus;
  }

  return expr;
}

static PExpr *ParsePredicate(char *str, u32 *index)
{
  PExpr *expr;
  i32 pred = pegMatch;

  if (str[*index] == '&') {
    pred = pegAssert;
    (*index)++;
    SkipSpacing(str, index);
  } else if (str[*index] == '!') {
    pred = pegRefute;
    (*index)++;
    SkipSpacing(str, index);
  }
  expr = ParsePrimary(str, index);
  if (expr) expr->predicate = pred;
  return expr;
}

static PExpr *ParseSequence(char *str, u32 *index)
{
  PExpr *seq;
  PExpr *expr = ParsePredicate(str, index);
  if (!expr) return expr;

  seq = NewPExpr(pegSeq);
  VecPush(seq->data.parts, expr);

  while (str[*index]) {
    expr = ParsePredicate(str, index);
    if (!expr) break;
    VecPush(seq->data.parts, expr);
  }

  if (VecCount(seq->data.parts) == 1) {
    expr = VecPop(seq->data.parts);
    FreePExpr(seq);
    return expr;
  }

  return seq;
}

static PExpr *ParseChoice(char *str, u32 *index)
{
  PExpr *choice;
  PExpr *expr = ParseSequence(str, index);
  if (!expr || str[*index] != '/') return expr;

  choice = NewPExpr(pegChoice);
  VecPush(choice->data.parts, expr);

  while (str[*index] == '/') {
    (*index)++;
    SkipSpacing(str, index);
    expr = ParseChoice(str, index);
    if (!expr) Fail(FreePExpr, choice);
    VecPush(choice->data.parts, expr);
  }

  return choice;
}

static PRule *NewPRule(char *name)
{
  PRule *rule = malloc(sizeof(PRule));
  rule->name = NewString(name);
  rule->expr = 0;
  rule->action = 0;
  return rule;
}

static void FreePRule(PRule *rule)
{
  free(rule->name);
  if (rule->expr) FreePExpr(rule->expr);
  free(rule);
}

static void PutRule(PRule *rule, Grammar *g)
{
  u32 key = HashStr(rule->name);
  HashMapSet(&g->map, key, VecCount(g->rules));
  VecPush(g->rules, rule);
}

void AddRule(char *name, char *expr, PRuleAction action, Grammar *g)
{
  PRule *rule = NewPRule(name);
  u32 index = 0;
  rule->expr = ParseChoice(expr, &index);
  rule->action = action;
  if (!rule->expr) {
    FreePRule(rule);
    fprintf(stderr, "Bad expression for \"%s\" at %d\n", name, index);
    return;
  }
  PutRule(rule, g);
}

void OnRule(char *name, PRuleAction action, Grammar *g)
{
  u32 key = HashStr(name);
  u32 index;
  if (HashMapFetch(&g->map, key, &index)) {
    g->rules[index]->action = action;
  }
}

typedef struct {
  Grammar *g;
  u32 index;
  char *text;
  u32 length;
  u32 longest;
} Parser;

static void InitParser(Parser *p, Grammar *g, char *text, u32 length)
{
  p->g = g;
  p->index = 0;
  p->text = text;
  p->length = length;
  p->longest = 0;
}

bool MatchExpr(PExpr *expr, PNode *node, Parser *p);

bool MatchRuleExpr(PExpr *expr, PNode *node, Parser *p)
{
  PNode *child = 0;
  u32 key = HashStr(expr->data.text);
  PRule *rule;
  u32 ruleIndex;
  if (!HashMapFetch(&p->g->map, key, &ruleIndex)) return false;
  rule = p->g->rules[ruleIndex];

  if (node) {
    child = NewPNode(expr->data.text);
    child->lexeme = p->text + p->index;
  }
  if (!MatchExpr(rule->expr, child, p)) {
    if (child) FreePNode(child);
    return false;
  }
  if (child) {
    child->length = p->text + p->index - child->lexeme;
    if (rule->action) child = rule->action(child);
    if (child) VecPush(node->elements, child);
  }
  return true;
}

bool MatchPrimary(PExpr *expr, PNode *node, Parser *p)
{
  u32 start = p->index;
  u32 i, len;
  u32 elemStart = node ? VecCount(node->elements) : 0;
  char ch;

  switch (expr->type) {
  case pegAny:
    if (p->index < p->length) {
      p->index++;
      return true;
    }
    return false;
  case pegLiteral:
    len = StrLen(expr->data.text);
    if (p->index + len <= p->length && Match(expr->data.text, p->text + p->index)) {
      p->index += len;
      return true;
    }
    return false;
  case pegClass:
    if (p->index == p->length) return false;
    ch = p->text[p->index];
    for (i = 0; i < VecCount(expr->data.ranges); i++) {
      if (ch >= expr->data.ranges[i].start && ch <= expr->data.ranges[i].end) {
        p->index++;
        return true;
      }
    }
    return false;
  case pegRule:
    if (MatchRuleExpr(expr, node, p)) {
      return true;
    } else {
      return false;
    }
  case pegSeq:
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      if (!MatchExpr(expr->data.parts[i], node, p)) {
        return false;
      }
    }
    return true;
  case pegChoice:
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      if (MatchExpr(expr->data.parts[i], node, p)) {
        return true;
      }
      if (node) TruncNodes(node->elements, elemStart);
      p->longest = Max(p->longest, p->index);
      p->index = start;
    }
    return false;
  }
}

bool MatchQuantity(PExpr *expr, PNode *node, Parser *p)
{
  u32 start = p->index;
  u32 elemStart = node ? VecCount(node->elements) : 0;
  bool match;

  switch (expr->quantity) {
  case pegOne:
    return MatchPrimary(expr, node, p);
  case pegOptional:
    if (!MatchPrimary(expr, node, p)) {
      p->index = start;
      if (node) TruncNodes(node->elements, elemStart);
    }
    return true;
  case pegZeroPlus:
    do {
      start = p->index;
      elemStart = node ? VecCount(node->elements) : 0;
      match = MatchPrimary(expr, node, p);
    } while (match);
    if (!match) {
      p->index = start;
      if (node) TruncNodes(node->elements, elemStart);
    }
    return true;
  case pegOnePlus:
    match = MatchPrimary(expr, node, p);
    if (!match) return false;
    while (match) {
      start = p->index;
      elemStart = node ? VecCount(node->elements) : 0;
      match = MatchPrimary(expr, node, p);
    }
    if (!match) {
      p->index = start;
      if (node) TruncNodes(node->elements, elemStart);
    }
    return true;
  }
}

bool MatchPredicate(PExpr *expr, PNode *node, Parser *p)
{
  u32 start = p->index;
  bool match;

  switch (expr->predicate) {
  case pegMatch:
    return MatchQuantity(expr, node, p);
  case pegAssert:
    match = MatchQuantity(expr, 0, p);
    p->index = start;
    return match;
  case pegRefute:
    match = MatchQuantity(expr, 0, p);
    p->index = start;
    return !match;
  }
}

bool MatchExpr(PExpr *expr, PNode *node, Parser *p)
{
  return MatchPredicate(expr, node, p);
}

PNode *ParseRule(char *name, char *text, u32 *index, u32 length, Grammar *g)
{
  Parser p;
  PNode *node;
  PRule *rule;
  u32 ruleIndex;
  u32 key = HashStr(name);
  if (!HashMapFetch(&g->map, key, &ruleIndex)) return 0;
  rule = g->rules[ruleIndex];

  InitParser(&p, g, text, length);

  node = NewPNode(name);
  node->lexeme = text + *index;
  if (!MatchExpr(rule->expr, node, &p)) {
    FreePNode(node);
    *index = p.longest;
    return 0;
  }

  node->length = text + *index - node->lexeme;
  if (rule->action) node = rule->action(node);
  *index = p.index;
  return node;
}

static PNode *IdentifierRule(PNode *node)
{
  PExpr *expr = NewPExpr(pegRule);
  u32 len = VecCount(node->elements);
  u32 i;
  expr->data.text = malloc(len + 1);
  expr->data.text[len] = 0;
  for (i = 0; i < len; i++) {
    PNode *child = node->elements[i];
    expr->data.text[i] = child->lexeme[0];
    FreePNode(child);
  }
  FreeVec(node->elements);
  node->elements = 0;
  node->value.data = expr;
  return node;
}

static PNode *LiteralRule(PNode *node)
{
  PExpr *expr = NewPExpr(pegLiteral);
  u32 len = VecCount(node->elements);
  u32 i;
  expr->data.text = malloc(len + 1);
  expr->data.text[len] = 0;
  for (i = 0; i < len; i++) {
    PNode *child = node->elements[i];
    u32 index = 0;
    ParseChar(child->lexeme, &index, &expr->data.text[i]);
    FreePNode(child);
  }
  FreeVec(node->elements);
  node->elements = 0;
  node->value.data = expr;
  return node;
}

static PNode *ClassRule(PNode *node)
{
  PExpr *expr = NewPExpr(pegClass);
  u32 i;
  expr->data.ranges = 0;
  for (i = 0; i < VecCount(node->elements); i++) {
    PNode *child = node->elements[i];
    PNode *charNode = child->elements[0];
    PRange range;
    u32 index = 0;
    ParseChar(charNode->lexeme, &index, &range.start);
    if (VecCount(child->elements) == 1) {
      range.end = range.start;
    } else {
      charNode = child->elements[1];
      index = 0;
      ParseChar(charNode->lexeme, &index, &range.end);
    }
    VecPush(expr->data.ranges, range);
  }

  node->value.data = expr;
  return node;
}

static PNode *DotRule(PNode *node)
{
  node->value.data = NewPExpr(pegAny);
  return node;
}

static PNode *SuffixRule(PNode *node)
{
  PNode *suffix;
  PNode *child = node->elements[0];
  PExpr *expr = child->value.data;

  if (VecCount(node->elements) == 1) return TransparentNode(node);

  node->elements[0] = 0;
  if (expr && VecCount(node->elements) == 2) {
    suffix = node->elements[1];
    if (StrEq(suffix->name, "STAR")) {
      expr->quantity = pegZeroPlus;
    } else if (StrEq(suffix->name, "PLUS")) {
      expr->quantity = pegOnePlus;
    } else if (StrEq(suffix->name, "QUESTION")) {
      expr->quantity = pegOptional;
    }
  }

  FreePNode(node);
  return child;
}

static PNode *PrefixRule(PNode *node)
{
  PNode *prefix;
  PNode *child;
  PExpr *expr;

  if (VecCount(node->elements) == 1) return TransparentNode(node);

  child = node->elements[1];
  node->elements[1] = 0;
  expr = child->value.data;

  if (expr) {
    prefix = node->elements[0];
    if (StrEq(prefix->name, "AND")) {
      expr->predicate = pegAssert;
    } else if (StrEq(prefix->name, "NOT")) {
      expr->predicate = pegRefute;
    }
  }

  FreePNode(node);
  return child;
}

static PNode *SequenceRule(PNode *node)
{
  u32 i;
  PExpr *expr;
  if (VecCount(node->elements) == 1) return TransparentNode(node);

  expr = NewPExpr(pegSeq);
  for (i = 0; i < VecCount(node->elements); i++) {
    PNode *child = node->elements[i];
    if (child->value.data) VecPush(expr->data.parts, child->value.data);
  }
  node->value.data = expr;
  return node;
}

static PNode *ChoiceRule(PNode *node)
{
  u32 i;
  PExpr *expr;
  if (VecCount(node->elements) == 1) return TransparentNode(node);

  expr = NewPExpr(pegChoice);
  for (i = 0; i < VecCount(node->elements); i++) {
    PNode *child = node->elements[i];
    if (child->value.data) VecPush(expr->data.parts, child->value.data);
  }
  node->value.data = expr;
  return node;
}

static PNode *RuleRule(PNode *node)
{
  PExpr *nameExpr = node->elements[0]->value.data;
  PRule *rule = NewPRule(nameExpr->data.text);
  rule->expr = node->elements[1]->value.data;
  FreePExpr(nameExpr);
  node->value.data = rule;
  return node;
}

static PNode *GrammarRule(PNode *node)
{
  Grammar *g = NewGrammar();
  u32 i;
  for (i = 0; i < VecCount(node->elements); i++) {
    PRule *rule = node->elements[i]->value.data;
    PutRule(rule, g);
  }
  node->value.data = g;
  return node;
}

static Grammar *PEGGrammar(void)
{
  Grammar *g = NewGrammar();
  AddRule("Grammar", "Spacing Definition+ EndOfFile", GrammarRule, g);
  AddRule("Definition", "Identifier LEFTARROW Expression Action?", RuleRule, g);
  AddRule("Action", "'<' Identifier '>' Spacing", 0, g);
  AddRule("Expression", "Sequence (SLASH Sequence)*", ChoiceRule, g);
  AddRule("Sequence", "Prefix*", SequenceRule, g);
  AddRule("Prefix", "(AND / NOT)? Suffix", PrefixRule, g);
  AddRule("Suffix", "Primary (QUESTION / STAR / PLUS)?", SuffixRule, g);
  AddRule("Primary", "Identifier !LEFTARROW / OPEN Expression CLOSE / Literal / Class / DOT", TransparentNode, g);
  AddRule("Identifier", "IdentStart IdentCont* Spacing", IdentifierRule, g);
  AddRule("IdentStart", "[a-zA-Z_]", 0, g);
  AddRule("IdentCont", "IdentStart / [0-9]", TransparentNode, g);
  AddRule("Literal", "['] (!['] Char)* ['] Spacing / [\"] (![\"] Char)* [\"] Spacing", LiteralRule, g);
  AddRule("Class", "'[' (!']' Range)* ']' Spacing", ClassRule, g);
  AddRule("Range", "Char '-' Char / Char", 0, g);
  AddRule("Char", "'\\\\' [nrt'\"\\[\\]\\\\] / '\\\\' [0-9A-F][0-9A-F] / !'\\\\' .", 0, g);
  AddRule("LEFTARROW", "'<-' Spacing", DiscardNode, g);
  AddRule("SLASH", "'/' Spacing", DiscardNode, g);
  AddRule("AND", "'&' Spacing", 0, g);
  AddRule("NOT", "'!' Spacing", 0, g);
  AddRule("QUESTION", "'?' Spacing", 0, g);
  AddRule("STAR", "'*' Spacing", 0, g);
  AddRule("PLUS", "'+' Spacing", 0, g);
  AddRule("OPEN", "'(' Spacing", DiscardNode, g);
  AddRule("CLOSE", "')' Spacing", DiscardNode, g);
  AddRule("DOT", "'.' Spacing", DotRule, g);
  AddRule("Spacing", "(Space / Comment)*", DiscardNode, g);
  AddRule("Comment", "'#' (!EndOfLine .)* EndOfLine", DiscardNode, g);
  AddRule("Space", "' ' / '\\t' / EndOfLine", DiscardNode, g);
  AddRule("EndOfLine", "'\\r\\n' / '\\n' / '\\r'", DiscardNode, g);
  AddRule("EndOfFile", "!.", DiscardNode, g);
  return g;
}

static char *CheckExpr(PExpr *expr, Grammar *g)
{
  u32 i;

  switch (expr->type) {
  case pegRule:
    if (!HashMapContains(&g->map, HashStr(expr->data.text))) {
      return expr->data.text;
    }
    return 0;
  case pegChoice:
  case pegSeq:
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      char *error = CheckExpr(expr->data.parts[i], g);
      if (error) return error;
    }
    return 0;
  default:
    return 0;
  }
}

static char *CheckGrammar(Grammar *g)
{
  u32 i;
  for (i = 0; i < VecCount(g->rules); i++) {
    char *error = CheckExpr(g->rules[i]->expr, g);
    if (error) return error;
  }
  return 0;
}

Grammar *ReadGrammar(char *text)
{
  Grammar *pegGrammar = PEGGrammar();
  Grammar *g;
  char *error;
  u32 length = StrLen(text);
  u32 i = 0;
  PNode *node = ParseRule("Grammar", text, &i, length, pegGrammar);
  FreeGrammar(pegGrammar);
  if (!node) {
    fprintf(stderr, "Error in grammar at %d\n", i);
    return 0;
  }

  g = node->value.data;
  FreePNode(node);
  error = CheckGrammar(g);
  if (error) {
    fprintf(stderr, "Undefined rule %s\n", error);
    FreeGrammar(g);
    return 0;
  }
  return g;
}

PNode *TransparentNode(PNode *node)
{
  if (VecCount(node->elements) == 1) {
    PNode *child = VecPop(node->elements);
    FreePNode(node);
    return child;
  }
  return node;
}

PNode *TransparentChild(PNode *node)
{
  if (VecCount(node->elements) == 1) {
    PNode *child = VecPop(node->elements);
    char *name = child->name;
    child->name = node->name;
    node->name = name;
    FreePNode(node);
    return child;
  }
  return node;
}

PNode *DiscardNode(PNode *node)
{
  FreePNode(node);
  return 0;
}

PNode *TextNode(PNode *node)
{
  char *text;
  u32 len = 0;
  u32 i;
  char *cur;

  for (i = 0; i < VecCount(node->elements); i++) {
    PNode *child = node->elements[i];
    len += child->length;
  }

  text = malloc(len + 1);
  text[len] = 0;
  cur = text;
  for (i = 0; i < VecCount(node->elements); i++) {
    PNode *child = node->elements[i];
    Copy(child->lexeme, cur, child->length);
    cur += child->length;
    FreePNode(child);
  }
  FreeVec(node->elements);
  node->elements = 0;
  node->value.data = text;
  return node;
}

#ifdef DEBUG
static void PrintChar(char ch, char *reserved)
{
  while (*reserved) {
    if (ch == *reserved) {
      printf("\\");
      printf("%c", ch);
      return;
    }
    reserved++;
  }

  switch (ch) {
  case '\n':
    printf("\\n");
    return;
  case '\r':
    printf("\\r");
    return;
  case '\t':
    printf("\\t");
    return;
  }

  if (IsPrintable(ch)) {
    printf("%c", ch);
  } else {
    printf("\\%02X", (u8)ch);
  }
}

void PrintPExpr(PExpr *expr)
{
  u32 i;

  if (expr->predicate == pegAssert) {
    printf("&");
  } else if (expr->predicate == pegRefute) {
    printf("!");
  }

  switch (expr->type) {
  case pegAny:
    printf(".");
    break;
  case pegLiteral:
    printf("'");
    for (i = 0; i < StrLen(expr->data.text); i++) {
      PrintChar(expr->data.text[i], "'\"\\");
    }
    printf("'");
    break;
  case pegClass:
    printf("[");
    for (i = 0; i < VecCount(expr->data.ranges); i++) {
      PrintChar(expr->data.ranges[i].start, "[]\\");
      if (expr->data.ranges[i].start != expr->data.ranges[i].end) {
        printf("-");
        PrintChar(expr->data.ranges[i].end, "[]\\");
      }
    }
    printf("]");
    break;
  case pegRule:
    printf("%s", expr->data.text);
    break;
  case pegSeq:
    if (expr->quantity != pegOne || expr->predicate != pegMatch) printf("(");
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      PExpr *part = expr->data.parts[i];
      bool needsParens = part->type == pegChoice &&
           part->quantity == pegOne &&
           part->predicate == pegMatch;
      if (needsParens) printf("(");
      PrintPExpr(expr->data.parts[i]);
      if (needsParens) printf(")");
      if (i < VecCount(expr->data.parts) - 1) {
        printf(" ");
      }
    }
    if (expr->quantity != pegOne || expr->predicate != pegMatch) printf(")");
    break;
  case pegChoice:
    if (expr->quantity != pegOne || expr->predicate != pegMatch) printf("(");
    for (i = 0; i < VecCount(expr->data.parts); i++) {
      PrintPExpr(expr->data.parts[i]);
      if (i < VecCount(expr->data.parts) - 1) {
        printf(" / ");
      }
    }
    if (expr->quantity != pegOne || expr->predicate != pegMatch) printf(")");
    break;
  }

  if (expr->quantity == pegZeroPlus) {
    printf("*");
  } else if (expr->quantity == pegOptional) {
    printf("?");
  } else if (expr->quantity == pegOnePlus) {
    printf("+");
  }
}

void PrintGrammar(Grammar *g)
{
  u32 i, j;
  u32 longestRule = 0;
  for (i = 0; i < VecCount(g->rules); i++) {
    longestRule = Max(longestRule, StrLen(g->rules[i]->name));
  }
  for (i = 0; i < VecCount(g->rules); i++) {
    printf("%s", g->rules[i]->name);
    for (j = StrLen(g->rules[i]->name); j < longestRule; j++) printf(" ");
    printf(" <- ");
    PrintPExpr(g->rules[i]->expr);
    printf("\n");
  }
}

static void PrintPNodeLevel(PNode *node, u32 level)
{
  u32 i;
  for (i = 0; i < level; i++) printf("  ");
  printf("%s [%d]", node->name, node->length);
  if (VecCount(node->elements) == 0) {
    printf(" %*.*s", node->length, node->length, node->lexeme);
  }
  printf("\n");
  for (i = 0; i < VecCount(node->elements); i++) {
    PrintPNodeLevel(node->elements[i], level+1);
  }
}

void PrintPNode(PNode *node)
{
  if (!node) return;
  PrintPNodeLevel(node, 0);
}
#endif
