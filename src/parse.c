#include "parse.h"
#include <univ.h>

#define IsIDStart(c)  (IsAlpha(c) || (c) == '_')
#define IsIDChar(c)   (IsIDStart(c) || IsDigit(c) || (c) == '?' || (c) == '!')

typedef struct {
  char *text;
  i32 length;
  i32 pos;
} Parser;
#define Peek(p)   ((p)->text[(p)->pos])
#define AtEnd(p)  ((p)->pos >= (p)->length)
#define Adv(p)    ((p)->pos++)

bool Match(char *test, Parser *p)
{
  i32 start = p->pos;
  while (*test && !AtEnd(p)) {
    if (*test != Peek(p)) {
      p->pos = start;
      return false;
    }
    Adv(p);
    test++;
  }
  return !(*test);
}

bool MatchKeyword(char *test, Parser *p)
{
  i32 start = p->pos;
  if (!Match(test, p)) return false;
  if (IsIDChar(Peek(p))) {
    p->pos = start;
    return false;
  }
  return true;
}

i32 MakeNode(char *type, i32 pos, i32 value)
{
  i32 node = Tuple(3);
  ObjSet(node, 0, SymVal(Symbol(type)));
  ObjSet(node, 1, IntVal(pos));
  ObjSet(node, 2, value);
  return node;
}

/*#define ParseError(msg, p)  MakeNode("error", (p)->pos, SymVal(Symbol(msg)))*/

i32 ParseError(char *msg, Parser *p)
{
  return MakeNode("error", p->pos, SymVal(Symbol(msg)));
}

i32 ParseProgram(Parser *p);
i32 ParseModname(Parser *p);
i32 ParseExports(Parser *p);
i32 ParseImport(Parser *p);
i32 ParseStmts(Parser *p);
i32 ParseStmt(Parser *p);
i32 ParseDef(Parser *p);
i32 ParseLet(Parser *p);
i32 ParseAssign(Parser *p);
i32 ParseExpr(Parser *p);
i32 ParseLambda(Parser *p);
i32 ParseLogic(Parser *p);
i32 ParseEqual(Parser *p);
i32 ParseCompare(Parser *p);
i32 ParseSum(Parser *p);
i32 ParseProduct(Parser *p);
i32 ParseUnary(Parser *p);
i32 ParseCall(Parser *p);
i32 ParseAccess(Parser *p);
i32 ParsePrimary(Parser *p);
i32 ParseGroup(Parser *p);
i32 ParseDo(Parser *p);
i32 ParseIf(Parser *p);
i32 ParseCond(Parser *p);
i32 ParseClause(Parser *p);
i32 ParseList(Parser *p);
i32 ParseTuple(Parser *p);
i32 ParseNum(Parser *p);
i32 ParseSymbol(Parser *p);
i32 ParseID(Parser *p);
i32 ParseIDList(Parser *p);
i32 ParseArgs(Parser *p);
void Spacing(Parser *p);
void VSpacing(Parser *p);

i32 Parse(char *text, i32 length)
{
  Parser p;
  p.text = text;
  p.length = length;
  p.pos = 0;
  return ParseProgram(&p);
}

i32 ParseProgram(Parser *p)
{
  i32 pos = p->pos, modname = 0, exports = 0, imports = 0, stmts = 0;
  VSpacing(p);
  if (MatchKeyword("module", p)) {
    modname = ParseModname(p);
    if (IsError(modname)) return modname;
    if (MatchKeyword("exports", p)) {
      exports = ParseExports(p);
      if (IsError(exports)) return exports;
    }
  }
  while (MatchKeyword("import", p)) {
    i32 import = ParseImport(p);
    if (IsError(import)) return import;
    imports = Pair(import, imports);
  }
  imports = MakeNode("imports", pos, ReverseList(imports));
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  stmts = MakeNode("do", pos, stmts);
  return MakeNode("module", pos, Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

i32 ParseModname(Parser *p)
{
  i32 id;
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!AtEnd(p) && !Match("\n", p) && !Match("\r", p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return id;
}

i32 ParseExports(Parser *p)
{
  i32 exports, pos;
  Spacing(p);
  pos = p->pos;
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !Match("\n", p) && !Match("\r", p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return MakeNode("exports", pos, exports);
}

i32 ParseImport(Parser *p)
{
  i32 id, alias, pos;
  Spacing(p);
  pos = p->pos;
  id = ParseID(p);
  if (IsError(id)) return id;
  if (MatchKeyword("as", p)) {
    Spacing(p);
    alias = ParseID(p);
    if (IsError(alias)) return alias;
  } else {
    alias = id;
  }
  if (!AtEnd(p) && !Match("\n", p) && !Match("\r", p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return MakeNode("import", pos, Pair(id, Pair(alias, 0)));
}

i32 ParseStmts(Parser *p)
{
  i32 stmts = 0, stmt;
  stmt = ParseStmt(p);
  if (IsError(stmt)) return stmt;
  stmts = Pair(stmt, stmts);
  while (!AtEnd(p) && IsNewline(Peek(p))) {
    Parser save = *p;
    VSpacing(p);
    stmt = ParseStmt(p);
    if (IsError(stmt)) {
      *p = save;
      break;
    }
    stmts = Pair(stmt, stmts);
  }
  VSpacing(p);
  return ReverseList(stmts);
}

i32 ParseStmt(Parser *p)
{
  if (MatchKeyword("def", p)) return ParseDef(p);
  if (MatchKeyword("let", p)) return ParseLet(p);
  return ParseExpr(p);
}

i32 ParseDef(Parser *p)
{
  i32 id, lambda, params = 0, body, pos = p->pos;
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!Match("(", p)) return ParseError("Expected '('", p);
  VSpacing(p);
  if (!Match(")", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match(")", p)) return ParseError("Expected ')'", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  lambda = MakeNode("->", pos, Pair(params, Pair(body, 0)));
  return MakeNode("def", pos, Pair(id, Pair(lambda, 0)));
}

i32 ParseLet(Parser *p)
{
  i32 assigns = 0, assign, pos = p->pos;
  Spacing(p);
  assign = ParseAssign(p);
  if (IsError(assign)) return assign;
  assigns = Pair(assign, assigns);
  while (!AtEnd(p) && Match(",", p)) {
    VSpacing(p);
    assign = ParseAssign(p);
    if (IsError(assign)) return assign;
    assigns = Pair(assign, assigns);
  }
  return MakeNode("let", pos, ReverseList(assigns));
}

i32 ParseAssign(Parser *p)
{
  i32 pos = p->pos, id, value;
  id = ParseID(p);
  if (IsError(id)) return id;
  Spacing(p);
  if (!Match("=", p)) return ParseError("Expected '='", p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsError(value)) return value;
  return MakeNode("=", pos, Pair(id, Pair(value, 0)));
}

i32 ParseExpr(Parser *p)
{
  if (Peek(p) == '\\') return ParseLambda(p);
  return ParseLogic(p);
}

i32 ParseLambda(Parser *p)
{
  i32 params = 0, body, pos = p->pos;
  if (!Match("\\", p)) return ParseError("Expected '\\'", p);
  Spacing(p);
  if (!Match("->", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match("->", p)) return ParseError("Expected '->'", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  return MakeNode("->", pos, Pair(params, Pair(body, 0)));
}

i32 ParseLogic(Parser *p)
{
  i32 expr = ParseEqual(p), pos, arg;
  char *op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = MatchKeyword("and", p) ? "and" : MatchKeyword("or", p) ? "or" : 0;
  while (op) {
    VSpacing(p);
    arg = ParseEqual(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = MatchKeyword("and", p) ? "and" : MatchKeyword("or", p) ? "or" : 0;
  }
  return expr;
}

i32 ParseEqual(Parser *p)
{
  i32 pos = p->pos, expr;
  expr = ParseCompare(p);
  if (IsError(expr)) return expr;
  if (!AtEnd(p) && Match("==", p)) {
    i32 arg;
    VSpacing(p);
    arg = ParseCompare(p);
    if (IsError(arg)) return arg;
    return MakeNode("==", pos, Pair(expr, Pair(arg, 0)));
  } else if (!AtEnd(p) && Match("!=", p)) {
    i32 arg;
    VSpacing(p);
    arg = ParseCompare(p);
    if (IsError(arg)) return arg;
    return MakeNode("==", pos, Pair(expr, Pair(arg, 0)));
  }
  return expr;
}

i32 ParseCompare(Parser *p)
{
  i32 expr = ParseSum(p), arg, pos;
  char *op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("<", p) ? "<" : Match(">", p) ? ">" : 0;
  while (op) {
    VSpacing(p);
    arg = ParseSum(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("<", p) ? "<" : Match(">", p) ? ">" : 0;
  }
  return expr;
}

i32 ParseSum(Parser *p)
{
  i32 expr = ParseProduct(p), arg, pos;
  char *op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("+", p) ? "+" : Match("-", p) ? "-" : 0;
  while (op) {
    VSpacing(p);
    arg = ParseProduct(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("+", p) ? "+" : Match("-", p) ? "-" : 0;
  }
  return expr;
}

i32 ParseProduct(Parser *p)
{
  i32 expr = ParseUnary(p), arg, pos;
  char *op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("*", p) ? "*" : Match("/", p) ? "/" : 0;
  while (op) {
    VSpacing(p);
    arg = ParseUnary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("*", p) ? "*" : Match("/", p) ? "/" : 0;
  }
  return expr;
}

i32 ParseUnary(Parser *p)
{
  i32 pos = p->pos, expr;
  if (Match("-", p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode("-", pos, Pair(expr, 0));
  } else if (MatchKeyword("not", p)) {
    Spacing(p);
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode("not", pos, Pair(expr, 0));
  } else {
    return ParseCall(p);
  }
}

i32 ParseCall(Parser *p)
{
  i32 expr = ParseAccess(p);
  if (IsError(expr)) return expr;
  while (!AtEnd(p) && Match("(", p)) {
    i32 pos = p->pos, args;
    VSpacing(p);
    args = 0;
    if (!Match(")", p)) {
      args = ParseArgs(p);
      if (IsError(args)) return args;
      expr = MakeNode("call", pos, Pair(expr, args));
      VSpacing(p);
      if (!Match(")", p)) return ParseError("Expected ')'", p);
    }
  }
  Spacing(p);
  return expr;
}

i32 ParseAccess(Parser *p)
{
  i32 expr = ParsePrimary(p);
  if (IsError(expr)) return expr;
  while (!AtEnd(p) && Match(".", p)) {
    i32 pos = p->pos;
    i32 arg = ParseID(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(".", pos, Pair(expr, Pair(arg, 0)));
  }
  return expr;
}

i32 ParsePrimary(Parser *p)
{
  if (Peek(p) == '(') {
    return ParseGroup(p);
  } else if (Peek(p) == '[') {
    return ParseList(p);
  } else if (Peek(p) == '{') {
    return ParseTuple(p);
  } else if (MatchKeyword("do", p)) {
    return ParseDo(p);
  } else if (MatchKeyword("if", p)) {
    return ParseIf(p);
  } else if (IsDigit(Peek(p))) {
    return ParseNum(p);
  } else if (Peek(p) == ':') {
    return ParseSymbol(p);
  } else if (IsIDStart(Peek(p))) {
    return ParseID(p);
  } else {
    return ParseError("Unexpected expression", p);
  }
}

i32 ParseGroup(Parser *p)
{
  i32 expr;
  if (!Match("(", p)) return ParseError("Expected '('", p);
  VSpacing(p);
  expr = ParseExpr(p);
  if (IsError(expr)) return expr;
  VSpacing(p);
  if (!Match(")", p)) return ParseError("Expected ')'", p);
  Spacing(p);
  return expr;
}

i32 ParseDo(Parser *p)
{
  i32 stmts, pos = p->pos;
  VSpacing(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!MatchKeyword("end", p)) return ParseError("Expected 'end'", p);
  Spacing(p);
  return MakeNode("do", pos, stmts);
}

i32 ParseIf(Parser *p)
{
  i32 items = 0;
  i32 item;
  i32 pos = p->pos;
  Spacing(p);
  item = ParseExpr(p);
  if (IsError(item)) return item;
  items = Pair(item, items);
  if (!MatchKeyword("do", p)) return ParseError("Expected 'do'", p);
  VSpacing(p);
  item = ParseStmts(p);
  if (IsError(item)) return item;
  items = Pair(item, items);
  if (MatchKeyword("else", p)) {
    VSpacing(p);
    item = ParseStmts(p);
    if (IsError(item)) return item;
    items = Pair(item, items);
  } else {
    item = MakeNode("nil", p->pos, 0);
    items = Pair(item, items);
  }
  if (!MatchKeyword("end", p)) return ParseError("Expected 'end'", p);
  Spacing(p);
  return MakeNode("if", pos, ReverseList(items));
}

i32 ParseList(Parser *p)
{
  i32 pos = p->pos;
  i32 items = 0;
  if (!Match("[", p)) return ParseError("Expected '['", p);
  VSpacing(p);
  if (!Match("]", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (Match("]", p)) return ParseError("Expected ']'", p);
  }
  Spacing(p);
  return MakeNode("tuple", pos, items);
}

i32 ParseTuple(Parser *p)
{
  i32 pos = p->pos, items = 0;
  if (!Match("{", p)) return ParseError("Expected '{'", p);
  VSpacing(p);
  if (!Match("}", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (!Match("}", p)) return ParseError("Expected '}'", p);
  }
  Spacing(p);
  return MakeNode("tuple", pos, items);
}

i32 ParseNum(Parser *p)
{
  /* TODO: detect overflow */
  i32 n = 0, pos = p->pos;
  if (!IsDigit(Peek(p))) return ParseError("Expected number", p);
  while (!AtEnd(p) && IsDigit(Peek(p))) {
    i32 d = Peek(p) - '0';
    n = n*10 + d;
    Adv(p);
  }
  Spacing(p);
  return MakeNode("num", pos, IntVal(n));
}

i32 ParseSymbol(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!Match(":", p)) return ParseError("Expected ':'", p);
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos, p->pos - pos);
  Spacing(p);
  return MakeNode("sym", pos, sym);
}

bool IsKeyword(u32 sym)
{
  if (sym == Symbol("module")) return true;
  if (sym == Symbol("exports")) return true;
  if (sym == Symbol("import")) return true;
  if (sym == Symbol("def")) return true;
  if (sym == Symbol("let")) return true;
  if (sym == Symbol("and")) return true;
  if (sym == Symbol("or")) return true;
  if (sym == Symbol("do")) return true;
  if (sym == Symbol("cond")) return true;
  if (sym == Symbol("if")) return true;
  if (sym == Symbol("else")) return true;
  if (sym == Symbol("end")) return true;
  return false;
}

i32 ParseID(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos, p->pos - pos);
  if (IsKeyword(sym)) return ParseError("Reserved word", p);
  Spacing(p);
  return MakeNode("id", pos, SymVal(sym));
}

i32 ParseIDList(Parser *p)
{
  i32 items = 0;
  i32 id = ParseID(p);
  if (IsError(id)) return id;
  items = Pair(id, items);
  while (!AtEnd(p) && Match(",", p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    items = Pair(id, items);
  }
  return ReverseList(items);
}

i32 ParseArgs(Parser *p)
{
  i32 items = 0;
  i32 item = ParseExpr(p);
  if (IsError(item)) return item;
  items = Pair(item, items);
  while (!AtEnd(p) && Match(",", p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsError(item)) return item;
    items = Pair(item, items);
  }
  return ReverseList(items);
}

void Spacing(Parser *p)
{
  while (!AtEnd(p) && IsSpace(Peek(p))) Adv(p);
  if (!AtEnd(p) && Peek(p) == ';') {
    while (!AtEnd(p) && !IsNewline(Peek(p))) Adv(p);
    Adv(p);
    Spacing(p);
  }
}

void VSpacing(Parser *p)
{
  Spacing(p);
  if (!AtEnd(p) && IsNewline(Peek(p))) {
    Adv(p);
    VSpacing(p);
  }
}

void PrintNode(i32 node, i32 level)
{
  u32 type = NodeType(node);
  i32 i;
  for (i = 0; i < level; i++) printf("  ");
  if (!type) {
    printf("nil:%d\n", NodePos(node));
    return;
  }

  printf("%s:%d", SymbolName(type), NodePos(node));
  if (type == Symbol("id")) {
    printf(" %s\n", SymbolName(RawVal(NodeValue(node))));
  } else if (type == Symbol("sym")) {
    printf(" :%s\n", SymbolName(RawVal(NodeValue(node))));
  } else if (type == Symbol("num")) {
    printf(" %d\n", RawVal(NodeValue(node)));
  } else if (type == Symbol("->")) {
    i32 params = NodeChild(node, 0);
    i32 body = NodeChild(node, 1);
    i32 numParams = ListLength(params);
    printf(" (");
    for (i = 0; i < numParams; i++) {
      printf("%s", SymbolName(RawVal(ListGet(params, i))));
      if (i < numParams-1) printf(", ");
    }
    printf(")\n");
    PrintNode(body, level+1);
  } else {
    i32 subnodes = NodeValue(node);
    printf("\n");
    while (subnodes && IsPair(subnodes)) {
      PrintNode(Head(subnodes), level+1);
      subnodes = Tail(subnodes);
    }
  }
}

void PrintAST(i32 node)
{
  PrintNode(node, 0);
}

void PrintError(i32 node, char *source)
{
  i32 pos = NodePos(node);
  i32 col = 0, i;
  i32 line = 0;
  char *start = source + pos;
  char *end = source + pos;
  while (start > source && !IsNewline(start[-1])) {
    col++;
    start--;
  }
  while (*end && !IsNewline(*end)) end++;
  for (i = 0; i < pos; i++) {
    if (IsNewline(source[i])) line++;
  }

  printf("%3d| %*.*s\n", line, (i32)(end-start), (i32)(end-start), start);
  printf("     ");
  for (i = 0; i < col; i++) printf(" ");
  printf("^\n");
  printf("%s\n", ErrorMsg(node));
}
