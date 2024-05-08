#include "parse.h"
#include <univ.h>

#define IsIDStart(c)  (IsAlpha(c) || (c) == '_')
#define IsIDChar(c)   (IsIDStart(c) || IsDigit(c) || (c) == '?' || (c) == '!')

typedef struct {
  char *text;
  i32 length;
  i32 pos;
} Parser;
#define Peek(p)       ((p)->text[(p)->pos])
#define AtEnd(p)      ((p)->pos >= (p)->length)
#define Adv(p)        ((p)->pos++)
#define Reset(p, pos) ((p)->pos = (pos), p)
#define MatchNL(p)    (Match("\n", p) || Match("\r", p))

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

bool CheckKeyword(char *test, Parser *p)
{
  i32 pos = p->pos;
  bool result = MatchKeyword(test, p);
  p->pos = pos;
  return result;
}

i32 MakeNode(i32 type, i32 pos, i32 value)
{
  i32 node = Tuple(3);
  ObjSet(node, 0, IntVal(type));
  ObjSet(node, 1, IntVal(pos));
  ObjSet(node, 2, value);
  return node;
}

#define ParseError(msg, p)  MakeNode(errorNode, (p)->pos, SymVal(Symbol(msg)))

i32 ParseModule(Parser *p);
i32 ParseModname(Parser *p);
i32 ParseExports(Parser *p);
i32 ParseImports(Parser *p);
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
i32 ParseClauses(Parser *p);
i32 ParseList(Parser *p);
i32 ParseTuple(Parser *p);
i32 ParseNum(Parser *p);
i32 ParseSymbol(Parser *p);
i32 ParseString(Parser *p);
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
  InitMem();
  return ParseModule(&p);
}

i32 ParseModule(Parser *p)
{
  i32 pos = p->pos, modname = 0, exports = 0, imports = 0, stmts = 0;
  VSpacing(p);
  if (CheckKeyword("module", p)) {
    modname = ParseModname(p);
    if (IsError(modname)) return modname;
    if (CheckKeyword("exports", p)) {
      exports = ParseExports(p);
      if (IsError(exports)) return exports;
    }
  }
  imports = ParseImports(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  return MakeNode(moduleNode, pos, Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

i32 ParseModname(Parser *p)
{
  i32 id;
  if (!MatchKeyword("module", p)) return ParseError("Expected \"module\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return id;
}

i32 ParseExports(Parser *p)
{
  i32 exports, pos;
  if (!MatchKeyword("exports", p)) return ParseError("Expected \"exports\"", p);
  Spacing(p);
  pos = p->pos;
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return MakeNode(exportNode, pos, NodeValue(exports));
}

i32 ParseImports(Parser *p)
{
  i32 imports = 0, start = p->pos;
  while (MatchKeyword("import", p)) {
    i32 import, id, alias;
    Spacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    if (MatchKeyword("as", p)) {
      Spacing(p);
      alias = ParseID(p);
      if (IsError(alias)) return alias;
    } else {
      alias = id;
    }
    import = Pair(id, Pair(alias, 0));
    imports = Pair(import, imports);
    if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
    VSpacing(p);
  }
  return MakeNode(importNode, start, ReverseList(imports));
}

i32 ParseStmts(Parser *p)
{
  i32 stmts = 0, stmt, pos = p->pos;
  stmt = ParseStmt(p);
  if (IsError(stmt)) return stmt;
  stmts = Pair(stmt, stmts);
  while (!AtEnd(p) && IsNewline(Peek(p))) {
    VSpacing(p);
    if (AtEnd(p) || CheckKeyword("end", p) || CheckKeyword("else", p)) break;
    stmt = ParseStmt(p);
    if (IsError(stmt)) return stmt;
    stmts = Pair(stmt, stmts);
  }
  VSpacing(p);
  return MakeNode(doNode, pos, ReverseList(stmts));
}

i32 ParseStmt(Parser *p)
{
  if (CheckKeyword("def", p)) return ParseDef(p);
  if (CheckKeyword("let", p)) return ParseLet(p);
  return ParseExpr(p);
}

i32 ParseDef(Parser *p)
{
  i32 id, lambda, params = 0, body, pos = p->pos;
  if (!MatchKeyword("def", p)) return ParseError("Expected \"def\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!Match("(", p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  if (!Match(")", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match(")", p)) return ParseError("Expected \")\"", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  lambda = MakeNode(lambdaNode, pos, Pair(params, Pair(body, 0)));
  return MakeNode(defNode, pos, Pair(id, Pair(lambda, 0)));
}

i32 ParseLet(Parser *p)
{
  i32 assigns = 0, assign, pos = p->pos;
  if (!MatchKeyword("let", p)) return ParseError("Expected \"let\"", p);
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
  return MakeNode(letNode, pos, ReverseList(assigns));
}

i32 ParseAssign(Parser *p)
{
  i32 id, value, pos = p->pos;
  id = ParseID(p);
  if (IsError(id)) return id;
  Spacing(p);
  if (!Match("=", p)) return ParseError("Expected \"=\"", p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsError(value)) return value;
  return MakeNode(listNode, pos, Pair(id, Pair(value, 0)));
}

i32 ParseExpr(Parser *p)
{
  if (Peek(p) == '\\') return ParseLambda(p);
  return ParseLogic(p);
}

i32 ParseLambda(Parser *p)
{
  i32 params = 0, body, pos = p->pos;
  if (!Match("\\", p)) return ParseError("Expected \"\\\"", p);
  Spacing(p);
  if (!Match("->", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match("->", p)) return ParseError("Expected \"->\"", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  return MakeNode(lambdaNode, pos, Pair(params, Pair(body, 0)));
}

i32 ParseLogic(Parser *p)
{
  i32 expr = ParseEqual(p), pos, arg;
  i32 op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = MatchKeyword("and", p) ? andNode : MatchKeyword("or", p) ? orNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseEqual(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = MatchKeyword("and", p) ? andNode : MatchKeyword("or", p) ? orNode : 0;
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
    return MakeNode(eqNode, pos, Pair(expr, Pair(arg, 0)));
  } else if (!AtEnd(p) && Match("!=", p)) {
    i32 arg;
    VSpacing(p);
    arg = ParseCompare(p);
    if (IsError(arg)) return arg;
    return MakeNode(notNode, pos, Pair(MakeNode(eqNode, pos, Pair(expr, Pair(arg, 0))), 0));
  }
  return expr;
}

i32 ParseCompare(Parser *p)
{
  i32 expr = ParseSum(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("<", p) ? ltNode : Match(">", p) ? gtNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseSum(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("<", p) ? ltNode : Match(">", p) ? gtNode : 0;
  }
  return expr;
}

i32 ParseSum(Parser *p)
{
  i32 expr = ParseProduct(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("+", p) ? addNode : Match("-", p) ? subNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseProduct(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("+", p) ? addNode : Match("-", p) ? subNode : 0;
  }
  return expr;
}

i32 ParseProduct(Parser *p)
{
  i32 expr = ParseUnary(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("*", p) ? mulNode : Match("/", p) ? divNode : Match("%", p) ? remNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseUnary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("*", p) ? mulNode : Match("/", p) ? divNode : Match("%", p) ? remNode : 0;
  }
  return expr;
}

i32 ParseUnary(Parser *p)
{
  i32 pos = p->pos, expr;
  if (Match("-", p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(negNode, pos, Pair(expr, 0));
  } else if (MatchKeyword("not", p)) {
    Spacing(p);
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(notNode, pos, Pair(expr, 0));
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
      expr = MakeNode(callNode, pos, Pair(expr, NodeValue(args)));
      VSpacing(p);
      if (!Match(")", p)) return ParseError("Expected \")\"", p);
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
    expr = MakeNode(accessNode, pos, Pair(expr, Pair(arg, 0)));
  }
  return expr;
}

i32 ParsePrimary(Parser *p)
{
  i32 pos = p->pos;
  if (Peek(p) == '(') {
    return ParseGroup(p);
  } else if (Peek(p) == '[') {
    return ParseList(p);
  } else if (Peek(p) == '{') {
    return ParseTuple(p);
  } else if (CheckKeyword("do", p)) {
    return ParseDo(p);
  } else if (CheckKeyword("if", p)) {
    return ParseIf(p);
  } else if (CheckKeyword("cond", p)) {
    return ParseCond(p);
  } else if (IsDigit(Peek(p))) {
    return ParseNum(p);
  } else if (Peek(p) == ':') {
    return ParseSymbol(p);
  } else if (Peek(p) == '"') {
    return ParseString(p);
  } else if (MatchKeyword("nil", p)) {
    return MakeNode(nilNode, pos, 0);
  } else if (IsIDStart(Peek(p))) {
    return ParseID(p);
  } else {
    return ParseError("Unexpected expression", p);
  }
}

i32 ParseGroup(Parser *p)
{
  i32 expr;
  if (!Match("(", p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  expr = ParseExpr(p);
  if (IsError(expr)) return expr;
  VSpacing(p);
  if (!Match(")", p)) return ParseError("Expected \")\"", p);
  Spacing(p);
  return expr;
}

i32 ParseDo(Parser *p)
{
  i32 stmts, pos = p->pos;
  if (!MatchKeyword("do", p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!MatchKeyword("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  if (!Tail(NodeValue(stmts))) return Head(NodeValue(stmts));
  return MakeNode(doNode, pos, NodeValue(stmts));
}

i32 ParseIf(Parser *p)
{
  i32 pred, cons, alt;
  i32 start = p->pos, pos;
  if (!MatchKeyword("if", p)) return ParseError("Expected \"if\"", p);
  Spacing(p);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  pos = p->pos;
  if (!MatchKeyword("do", p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  cons = ParseStmts(p);
  if (IsError(cons)) return cons;
  if (!Tail(NodeValue(cons))) cons = Head(NodeValue(cons));
  pos = p->pos;
  if (MatchKeyword("else", p)) {
    VSpacing(p);
    alt = ParseStmts(p);
    if (IsError(alt)) return alt;
    if (!Tail(NodeValue(alt))) alt = Head(NodeValue(alt));
  } else {
    alt = MakeNode(nilNode, pos, 0);
  }
  if (!MatchKeyword("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return MakeNode(ifNode, start, Pair(pred, Pair(cons, Pair(alt, 0))));
}

i32 ParseCond(Parser *p)
{
  i32 expr;
  if (!MatchKeyword("cond", p)) return ParseError("Expected \"cond\"", p);
  expr = ParseClauses(p);
  if (IsError(expr)) return expr;
  if (!Match("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return expr;
}

i32 ParseClauses(Parser *p)
{
  i32 pred, cons, alt, pos = p->pos;
  VSpacing(p);
  if (CheckKeyword("end", p)) return MakeNode(nilNode, p->pos, 0);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  if (!Match("->", p)) return ParseError("Expected \"->\"", p);
  VSpacing(p);
  cons = ParseExpr(p);
  if (IsError(cons)) return cons;
  alt = ParseClauses(p);
  if (IsError(alt)) return alt;
  return MakeNode(ifNode, pos, Pair(pred, Pair(cons, Pair(alt, 0))));
}

i32 ParseList(Parser *p)
{
  i32 pos = p->pos;
  i32 items = 0;
  if (!Match("[", p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (!Match("]", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (Match("]", p)) return ParseError("Expected \"]\"", p);
  }
  Spacing(p);
  return MakeNode(listNode, pos, NodeValue(items));
}

i32 ParseTuple(Parser *p)
{
  i32 pos = p->pos, items = 0;
  if (!Match("{", p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (!Match("}", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (!Match("}", p)) return ParseError("Expected \"}\"", p);
  }
  Spacing(p);
  return MakeNode(tupleNode, pos, NodeValue(items));
}

i32 ParseNum(Parser *p)
{
  i32 n = 0, pos = p->pos;
  if (!IsDigit(Peek(p))) return ParseError("Expected number", p);
  while (!AtEnd(p) && IsDigit(Peek(p))) {
    i32 d = Peek(p) - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
    Adv(p);
  }
  Spacing(p);
  return MakeNode(numNode, pos, IntVal(n));
}

i32 ParseSymbol(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!Match(":", p)) return ParseError("Expected \":\"", p);
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos, p->pos - pos);
  Spacing(p);
  return MakeNode(symNode, pos, SymVal(sym));
}

i32 ParseString(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!Match("\"", p)) return ParseError("Expected quote", p);
  while (!AtEnd(p) && Peek(p) != '"') {
    if (Peek(p) == '\\') Adv(p);
    Adv(p);
  }
  sym = SymbolFrom(p->text + pos + 1, p->pos - pos - 1);
  if (!Match("\"", p)) return ParseError("Expected quote", p);
  Spacing(p);
  return MakeNode(strNode, pos, SymVal(sym));
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
  if (sym == Symbol("not")) return true;
  return false;
}

i32 ParseID(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos, p->pos - pos);
  if (IsKeyword(sym)) return ParseError("Reserved word", Reset(p, pos));
  Spacing(p);
  return MakeNode(idNode, pos, SymVal(sym));
}

i32 ParseIDList(Parser *p)
{
  i32 items = 0;
  i32 pos = p->pos;
  i32 id = ParseID(p);
  if (IsError(id)) return id;
  items = Pair(id, items);
  while (!AtEnd(p) && Match(",", p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    items = Pair(id, items);
  }
  return MakeNode(listNode, pos, ReverseList(items));
}

i32 ParseArgs(Parser *p)
{
  i32 items = 0;
  i32 item = ParseExpr(p);
  i32 pos = p->pos;
  if (IsError(item)) return item;
  items = Pair(item, items);
  while (!AtEnd(p) && Match(",", p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsError(item)) return item;
    items = Pair(item, items);
  }
  return MakeNode(listNode, pos, ReverseList(items));
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

char *NodeName(i32 type)
{
  switch (type) {
  case errorNode:   return "error";
  case nilNode:     return "nil";
  case idNode:      return "id";
  case numNode:     return "num";
  case symNode:     return "sym";
  case strNode:     return "str";
  case listNode:    return "list";
  case tupleNode:   return "tuple";
  case doNode:      return "do";
  case ifNode:      return "if";
  case andNode:     return "and";
  case orNode:      return "or";
  case eqNode:      return "eq";
  case ltNode:      return "lt";
  case gtNode:      return "gt";
  case addNode:     return "add";
  case subNode:     return "sub";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case negNode:     return "neg";
  case notNode:     return "not";
  case callNode:    return "call";
  case accessNode:  return "access";
  case lambdaNode:  return "lambda";
  case letNode:     return "let";
  case defNode:     return "def";
  case importNode:  return "import";
  case exportNode:  return "export";
  case moduleNode:  return "module";
  default:          return "?";
  }
}

void PrintNode(i32 node, i32 level, u32 lines)
{
  i32 type = NodeType(node);
  i32 i, subnodes;

  if (!type) {
    printf("nil:%d\n", NodePos(node));
    return;
  }

  printf("%s:%d", NodeName(type), NodePos(node));
  switch (type) {
  case idNode:
    printf(" %s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case strNode:
    printf(" \"%s\"\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case symNode:
    printf(" :%s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case numNode:
    printf(" %d\n", RawVal(NodeValue(node)));
    break;
  case letNode:
    printf("\n");
    break;
  case importNode:
    printf("\n");
    break;
  default:
    subnodes = NodeValue(node);
    printf("\n");
    lines |= Bit(level);
    while (subnodes && IsPair(subnodes)) {
      for (i = 0; i < level; i++) {
        if (lines & Bit(i)) {
          printf("│ ");
        } else {
          printf("  ");
        }
      }
      if (Tail(subnodes)) {
        printf("├ ");
      } else {
        lines &= ~Bit(level);
        printf("└ ");
      }
      PrintNode(Head(subnodes), level+1, lines);
      subnodes = Tail(subnodes);
    }
  }
}

void PrintAST(i32 node)
{
  PrintNode(node, 0, 0);
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

  printf("%sError: %s\n", ANSIRed, ErrorMsg(node));
  printf("%3d| %*.*s\n", line+1, (i32)(end-start), (i32)(end-start), start);
  printf("     ");
  for (i = 0; i < col; i++) printf(" ");
  printf("^\n%s", ANSINormal);
}
