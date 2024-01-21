/*
The compiler takes a ModuleNode AST and compiles it into bytecode, which is
appended to a chunk. Named modules are compiled into a lambda, which is defined
in the module frame, while the main module is compiled directly. Each module's
imports are compiled, then its body, with each node type generating specific
code (described below).

Most compile functions take a "linkage" parameter, which is true if the node is
part of a returned expression. This allows the compiler to skip generating a
call stack frame for tail calls, which allows efficient tail-call recursion.

The compiler keeps track of the the current environment and the symbols defined
in it. When a variable lookup is compiled, the VM only needs the relative
environment position, not the symbol, making lookups faster. This also lets the
compiler detect undefined variables.

The compiler updates the chunk's file map for each compiled module and source
map for each compiled node. It also copies symbols from the compiler symbol
table to the chunk's symbol table for filenames, exported variables, literal
symbols, and literal strings.
*/

#include "compile.h"
#include "project.h"
#include "runtime/ops.h"
#include "runtime/primitives.h"

#define LinkNext false
#define LinkReturn true

static Result CompileImports(Node *imports, Compiler *c);
static Result CompileExpr(Node *node, bool linkage, Compiler *c);
static void CompileLinkage(bool linkage, Compiler *c);
static Result CompileError(char *message, Compiler *c);
static void PushNodePos(Node *node, Compiler *c);
static u32 LambdaBegin(u32 num_params, Compiler *c);
static void LambdaEnd(u32 ref, Compiler *c);
#define CompileOk()  ValueResult(Nil)

void InitCompiler(Compiler *c, SymbolTable *symbols, ObjVec *modules,
                  HashMap *mod_map, Chunk *chunk)
{
  c->pos = 0;
  c->env = 0;
  c->filename = 0;
  c->symbols = symbols;
  c->modules = modules;
  c->mod_map = mod_map;
  c->chunk = chunk;
  c->mod_num = 0;
  c->import_num = 0;
}

/* If there are multiple modules in a chunk, the first thing a program does is
extend the base environment (which defines the primitives) with a new frame,
where the modules will be defined. */
Result CompileModuleFrame(u32 num_modules, Compiler *c)
{
  if (num_modules > 0) {
    PushFilePos(Sym("*init*", &c->chunk->symbols), c->chunk);
    PushConst(IntVal(num_modules), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
  }

  return CompileOk();
}

/* Compiles a module. Imports are compiled, followed by the module statements.
The main module ends with a "halt" instruction, while other modules are wrapped
in a lambda and defined in the module frame.

  ; for normal modules:
  const body
  const 0   ; modules have no parameters
  lambda
  const after
  jump
body:
  <imports code>
  <body code>
  export    ; discard imports frame (omitted if no imports)
  pop
  return
after:
  const <module num>
  define

  ; for the main module:
  <imports code>
  <body code>
  export    ; discard imports frame (omitted if no imports)
  pop
  halt
*/
Result CompileModule(Node *module, Compiler *c)
{
  Result result;
  Node *imports = ModuleImports(module);
  Node *body = ModuleBody(module);
  u32 num_imports = NumNodeChildren(imports);
  u32 lambda_ref;

  /* copy filename symbol to chunk */
  c->filename = SymbolName(ModuleFile(module), c->symbols);
  Sym(c->filename, &c->chunk->symbols);

  /* record start position of module in chunk */
  c->pos = 0;
  PushFilePos(ModuleFile(module), c->chunk);

  /* create lambda for module */
  if (ModuleName(module) != MainModule) {
    lambda_ref = LambdaBegin(0, c);
  }

  /* compile imports */
  if (num_imports > 0) {
    result = CompileImports(imports, c);
    if (!result.ok) return result;
  }

  result = CompileExpr(body, LinkNext, c);
  if (!result.ok) return result;
  PushNodePos(ModuleFilename(module), c);

  /* discard import frame */
  if (num_imports > 0) {
    PushByte(OpExport, c->chunk);
    PushByte(OpPop, c->chunk);
    c->env = PopFrame(c->env);
  }

  if (ModuleName(module) != MainModule) {
    PushByte(OpReturn, c->chunk);
    LambdaEnd(lambda_ref, c);

    /* define module as the lambda */
    PushConst(IntVal(c->mod_num), c->chunk);
    PushByte(OpDefine, c->chunk);
    c->mod_num++;
  } else {
    PushByte(OpHalt, c->chunk);
  }

  return CompileOk();
}

static u32 CountExports(Node *imports, Compiler *c);
static Result CompileImport(Node *name, Node *alias, Compiler *c);
static u32 DefineImports(Node *imports, Compiler *c);

/* Extends the environment for the imports, then compiles each one. The extended
frame is discarded elsewhere.

  const <num_imports>
  tuple
  extend
  ; for each import:
  <import code>
*/
static Result CompileImports(Node *imports, Compiler *c)
{
  u32 num_imports = CountExports(imports, c);
  u32 i;

  /* extend env for imports */
  PushConst(IntVal(num_imports), c->chunk);
  PushByte(OpTuple, c->chunk);
  PushByte(OpExtend, c->chunk);
  c->env = ExtendFrame(c->env, num_imports);
  c->import_num = 0;

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Node *node = NodeChild(imports, i);
    Result result;

    PushNodePos(node, c);
    result = CompileImport(NodeChild(node, 0), NodeChild(node, 1), c);
    if (!result.ok) return result;
  }

  return CompileOk();
}

/* Counts the number of imported symbols in a module. This is the number of
aliased imports plus the total number of exports from unqualified imports (e.g.
`import X as *`). */
static u32 CountExports(Node *imports, Compiler *c)
{
  u32 count = 0;
  u32 i;
  for (i = 0; i < NumNodeChildren(imports); i++) {
    Node *import = NodeChild(imports, i);
    Val mod_name = NodeValue(NodeChild(import, 0));
    Val alias = NodeValue(NodeChild(import, 1));

    if (alias != Nil) {
      count++;
    } else if (HashMapContains(c->mod_map, mod_name)) {
      Node *module = VecRef(c->modules, HashMapGet(c->mod_map, mod_name));
      Node *exports = ModuleExports(module);
      count += NumNodeChildren(exports);
    }
  }
  return count;
}

/* An import calls the module as a function, and either defines the result as
the module's alias or defines each export directly.

  const after
  link
  const <module env position>
  lookup
  const 0
  apply

  ; if module is aliased:
  const <next assignment slot>
  define
  ; otherwise, see DefineImports
*/
static Result CompileImport(Node *name, Node *alias, Compiler *c)
{
  Val name_val = NodeValue(name);
  Val alias_val = NodeValue(alias);
  i32 def, link, link_ref;

  if (!HashMapContains(c->mod_map, name_val)) {
    return CompileError("Undefined module", c);
  }

  def = FrameFind(c->env, name_val);
  if (def < 0) return CompileError("Undefined module", c);

  link = PushConst(IntVal(0), c->chunk);
  link_ref = PushByte(OpLink, c->chunk);
  PushConst(IntVal(def), c->chunk);
  PushByte(OpLookup, c->chunk);

  PushConst(IntVal(0), c->chunk);
  PushByte(OpApply, c->chunk);
  PatchConst(c->chunk, link, link_ref);

  if (alias_val == Nil) {
    /* import directly into module namespace */
    Node *import_mod = VecRef(c->modules, HashMapGet(c->mod_map, name_val));
    Node *imported_vals = ModuleExports(import_mod);
    c->import_num += DefineImports(imported_vals, c);
  } else {
    /* import map */
    PushConst(IntVal(c->import_num), c->chunk);
    PushByte(OpDefine, c->chunk);
    FrameSet(c->env, c->import_num, alias_val);
    c->import_num++;
  }

  return CompileOk();
}

/* Extracts and defines each import symbol from an exported map

  ; for each exported var from the import:
  dup   ; duplicate the exported map (omitted for the last item)
  const <var>
  swap  ; swap args to access the map as a function call
  const 1
  apply
  const <next assignment slot>
  define
*/
static u32 DefineImports(Node *imports, Compiler *c)
{
  u32 i;

  for (i = 0; i < NumNodeChildren(imports); i++) {
    Val import = NodeValue(NodeChild(imports, i));

    if (i < NumNodeChildren(imports) - 1) {
      /* keep map for future iterations */
      PushByte(OpDup, c->chunk);
    }

    /* access map with import name key */
    PushConst(import, c->chunk);
    PushByte(OpSwap, c->chunk);
    PushConst(IntVal(1), c->chunk);
    PushByte(OpApply, c->chunk);

    PushConst(IntVal(c->import_num + i), c->chunk);
    PushByte(OpDefine, c->chunk);
    FrameSet(c->env, c->import_num + i, import);
  }
  return NumNodeChildren(imports);
}

static Result CompileDo(Node *expr, bool linkage, Compiler *c);
static Result CompileExport(Node *node, bool linkage, Compiler *c);
static Result CompileCall(Node *call, bool linkage, Compiler *c);
static Result CompileSet(Node *node, bool linkage, Compiler *c);
static Result CompileLambda(Node *expr, bool linkage, Compiler *c);
static Result CompileIf(Node *expr, bool linkage, Compiler *c);
static Result CompileAnd(Node *expr, bool linkage, Compiler *c);
static Result CompileOr(Node *expr, bool linkage, Compiler *c);
static Result CompileList(Node *items, bool linkage, Compiler *c);
static Result CompileTuple(Node *items, bool linkage, Compiler *c);
static Result CompileMap(Node *items, bool linkage, Compiler *c);
static Result CompileString(Node *sym, bool linkage, Compiler *c);
static Result CompileVar(Node *id, bool linkage, Compiler *c);
static Result CompileConst(Node *value, bool linkage, Compiler *c);

static Result CompileExpr(Node *node, bool linkage, Compiler *c)
{
  PushNodePos(node, c);

  switch (node->type) {
  case NilNode:           return CompileConst(node, linkage, c);
  case IDNode:            return CompileVar(node, linkage, c);
  case NumNode:           return CompileConst(node, linkage, c);
  case StringNode:        return CompileString(node, linkage, c);
  case SymbolNode:        return CompileConst(node, linkage, c);
  case ListNode:          return CompileList(node, linkage, c);
  case TupleNode:         return CompileTuple(node, linkage, c);
  case MapNode:           return CompileMap(node, linkage, c);
  case AndNode:           return CompileAnd(node, linkage, c);
  case OrNode:            return CompileOr(node, linkage, c);
  case IfNode:            return CompileIf(node, linkage, c);
  case DoNode:            return CompileDo(node, linkage, c);
  case CallNode:          return CompileCall(node, linkage, c);
  case LambdaNode:        return CompileLambda(node, linkage, c);
  case SetNode:           return CompileSet(node, linkage, c);
  case ExportNode:        return CompileExport(node, linkage, c);
  default:                return CompileError("Unknown expression", c);
  }
}

/* A DoNode introduces a new lexical scope if it contains any assignments, and
compiles each statement in sequence. At the beginning of the block, all DefNode
variables are defined to allow recursion. Each statement's result except the
last is discarded. If the last statement is an assignment, its result is `nil`
(assignment otherwise doesn't produce a result).

  const <num_assigns>     ; env extension is omitted if there are no assigns
  tuple
  extend
  ; for each statement:
  ; if the statement is a LetNode or DefNode:
  <code for value>
  const <next assignment slot>
  define
  ; if the assignment is the last statement, a `nil` value is pushed here
  ; normal statements:
  <statement code>
  pop   ; omitted from the last statement

  ; discard assignment frame (omitted if there are no assigns)
  export
  pop
*/
static Result CompileDo(Node *node, bool linkage, Compiler *c)
{
  Node *assigns = NodeChild(node, 0);
  Node *stmts = NodeChild(node, 1);
  u32 num_assigns = NumNodeChildren(assigns);
  u32 assign_num = 0;
  u32 i;

  /* extend env for assigns */
  if (num_assigns > 0) {
    PushConst(IntVal(num_assigns), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
    c->env = ExtendFrame(c->env, num_assigns);

    /* pre-define all def statements */
    assign_num = 0;
    for (i = 0; i < NumNodeChildren(stmts); i++) {
      Node *stmt = NodeChild(stmts, i);
      Val var;

      /* keep the assignment slots aligned */
      if (stmt->type == LetNode) {
        assign_num++;
        continue;
      }
      if (stmt->type != DefNode) continue;

      var = NodeValue(NodeChild(stmt, 0));
      FrameSet(c->env, assign_num, var);
      assign_num++;
    }
  }

  assign_num = 0;
  for (i = 0; i < NumNodeChildren(stmts); i++) {
    Node *stmt = NodeChild(stmts, i);
    Result result;
    bool is_last = i == NumNodeChildren(stmts) - 1;
    bool stmt_linkage = is_last ? linkage : LinkNext;

    if (stmt->type == LetNode || stmt->type == DefNode) {
      Node *var = NodeChild(stmt, 0);
      Node *value = NodeChild(stmt, 1);

      result = CompileExpr(value, stmt_linkage, c);
      if (!result.ok) return result;

      PushNodePos(var, c);
      PushConst(IntVal(assign_num), c->chunk);
      PushByte(OpDefine, c->chunk);
      FrameSet(c->env, assign_num, NodeValue(var));
      assign_num++;
      if (is_last) PushConst(Nil, c->chunk);
    } else {
      result = CompileExpr(stmt, stmt_linkage, c);
      if (!result.ok) return result;
      if (!is_last) PushByte(OpPop, c->chunk);
    }
  }

  /* discard frame for assigns */
  PushNodePos(node, c);
  if (num_assigns > 0) {
    PushByte(OpExport, c->chunk);
    PushByte(OpPop, c->chunk);
    c->env = PopFrame(c->env);
  }

  return CompileOk();
}

/* An ExportNode takes the current env frame and creates a map of each assigned
variable. It then redefines the current module as the exported map.

  map
  ; for each assigned variable n, with matching var:
  const <n>
  lookup
  const <var>
  put
  dup   ; duplicate map to redefine it and return it
  const <module env position>
  define
*/
static Result CompileExport(Node *node, bool linkage, Compiler *c)
{
  u32 num_exports = NumNodeChildren(node);
  u32 exports_frame;

  PushByte(OpMap, c->chunk);
  if (num_exports > 0) {
    u32 i;

    for (i = 0; i < num_exports; i++) {
      Val name = NodeValue(NodeChild(node, i));
      Sym(SymbolName(name, c->symbols), &c->chunk->symbols);
      PushConst(IntVal(i), c->chunk);
      PushByte(OpLookup, c->chunk);
      PushConst(name, c->chunk);
      PushByte(OpPut, c->chunk);
    }
  }

  PushByte(OpDup, c->chunk);

  /* redefine module as exported value */
  exports_frame = ExportsFrame(c->env);
  PushConst(IntVal(exports_frame + c->mod_num), c->chunk);
  PushByte(OpDefine, c->chunk);
  CompileLinkage(linkage, c);

  return CompileOk();
}

static bool IsPrimitiveCall(Node *op, Compiler *c)
{
  /* primitive calls are calls from the highest env frame */
  if (op->type != IDNode) return false;
  return FrameNum(c->env, NodeValue(op)) == 0;
}

/* Each argument in a call is evaluated left-to-right (placing them in reverse
order on the stack), followed by the operator. Then OpApply calls the function.
If the node is within a tail node, the stack should already be set up with the
correct return address before this node is evaluated, so no linking is done—this
implements tail-call recursion. Otherwise, before the call is compiled, the link
information is compiled to return to the position after the call. Primitive
functions don't require linkage info, since they don't jump anywhere, but a
return instruction must be added if the node is in a tail node.

  const <after>   ; linkage if not a tail call
  link
  ; for each argument n:
  <code for n>
  <code for operator>
  const <num_args>
  apply
after:
  ; if the call is primitive and a tail call, a return op is added here
*/
static Result CompileCall(Node *node, bool linkage, Compiler *c)
{
  Node *op = NodeChild(node, 0);
  u32 num_args = NumNodeChildren(node) - 1;
  u32 i, link, link_ref;
  Result result;
  bool is_primitive;

  is_primitive = IsPrimitiveCall(op, c);

  if (linkage == LinkNext && !is_primitive) {
    link = PushConst(IntVal(0), c->chunk);
    link_ref = PushByte(OpLink, c->chunk);
  }

  for (i = 0; i < num_args; i++) {
    Node *arg = NodeChild(node, i + 1);

    result = CompileExpr(arg, LinkNext, c);
    if (!result.ok) return result;
  }

  result = CompileExpr(op, LinkNext, c);
  if (!result.ok) return result;

  PushConst(IntVal(num_args), c->chunk);
  PushByte(OpApply, c->chunk);

  if (linkage == LinkNext && !is_primitive) {
    PatchConst(c->chunk, link, link_ref);
  }

  if (is_primitive) {
    CompileLinkage(linkage, c);
  }

  return CompileOk();
}

/* Redefines a variable to the value on the stack

  dup
  const <var>
  define
*/
static Result CompileSet(Node *node, bool linkage, Compiler *c)
{
  u32 num_args = NumNodeChildren(node) - 1;
  Node *var, *val;
  Val id;
  i32 def;
  Result result;

  if (num_args != 2) {
    return CompileError("Wrong number of arguments: Expected 2", c);
  }

  var = NodeChild(node, 1);
  val = NodeChild(node, 2);

  if (var->type != IDNode) {
    return CompileError("Expected variable", c);
  }

  id = NodeValue(var);
  def = FrameFind(c->env, id);
  if (def == -1) return CompileError("Undefined variable", c);

  result = CompileExpr(val, LinkNext, c);
  if (!result.ok) return result;

  PushByte(OpDup, c->chunk);
  PushConst(IntVal(def), c->chunk);
  PushByte(OpDefine, c->chunk);

  CompileLinkage(linkage, c);
  return CompileOk();
}

/* A lambda creates a function, whose body is compiled just afterwards. If the
lambda has any parameters, the body defines them in a new frame. Parameters are
passed in reverse order on the stack, so the lambda defines them in reverse to
place them in the frame in the order they appear in the source code. The body is
compiled as a tail node, which results in a return op at the end of the body.

  const body
  const <num_params>
  lambda
  const after
  jump
body:
  <num_params>
  tuple
  extend

  ; for each param n:
  const <num_params - n - 1>
  define

  <lambda body>
after:
*/
static Result CompileLambda(Node *node, bool linkage, Compiler *c)
{
  Node *params = NodeChild(node, 0);
  Node *body = NodeChild(node, 1);
  Result result;
  u32 lambda_ref, i;
  u32 num_params = NumNodeChildren(params);

  /* create lambda */
  lambda_ref = LambdaBegin(num_params, c);

  if (num_params > 0) {
    PushNodePos(params, c);
    PushConst(IntVal(num_params), c->chunk);
    PushByte(OpTuple, c->chunk);
    PushByte(OpExtend, c->chunk);
    c->env = ExtendFrame(c->env, num_params);

    for (i = 0; i < num_params; i++) {
      Node *param = NodeChild(params, i);
      Val id = NodeValue(NodeChild(params, i));
      PushNodePos(param, c);
      FrameSet(c->env, i, id);
      PushConst(IntVal(num_params - i - 1), c->chunk);
      PushByte(OpDefine, c->chunk);
    }
  }

  result = CompileExpr(body, LinkReturn, c);
  if (!result.ok) return result;

  LambdaEnd(lambda_ref, c);

  CompileLinkage(linkage, c);

  if (num_params > 0) c->env = PopFrame(c->env);

  return CompileOk();
}

/*
  <predicate code>
  const true_branch
  ; false branch:
  pop   ; discard predicate result
  <alternative code>
  const after   ; if linkage is return, this jump is omitted
  jump
true_branch:
  pop   ; discard predicate result
  <consequent code>
after:
*/
static Result CompileIf(Node *node, bool linkage, Compiler *c)
{
  Node *predicate = NodeChild(node, 0);
  Node *consequent = NodeChild(node, 1);
  Node *alternative = NodeChild(node, 2);
  Result result;
  u32 branch, branch_ref, jump, jump_ref;

  result = CompileExpr(predicate, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(alternative, linkage, c);
  if (!result.ok) return result;
  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->chunk);
    jump_ref = PushByte(OpJump, c->chunk);
  }

  PatchConst(c->chunk, branch, branch_ref);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(consequent, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump, jump_ref);
  }

  return CompileOk();
}

/* For `a and b`:

  <a code>
  const next
  branch
  ; a is false:
  const after   ; if linkage is return, a return op is emitted here instead
  jump
next:
  pop   ; discard a result
  <b code>
after:
*/
static Result CompileAnd(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch, branch_ref, jump, jump_ref;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  if (linkage == LinkNext) {
    jump = PushConst(IntVal(0), c->chunk);
    jump_ref = PushByte(OpJump, c->chunk);
  } else {
    CompileLinkage(linkage, c);
  }

  PatchConst(c->chunk, branch, branch_ref);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  if (linkage == LinkNext) {
    PatchConst(c->chunk, jump, jump_ref);
  }

  return CompileOk();
}

/* For `a or b`:

  <a code>
  const after
  branch
  pop   ; discard a result
  <b code>
after:
*/
static Result CompileOr(Node *node, bool linkage, Compiler *c)
{
  Node *a = NodeChild(node, 0);
  Node *b = NodeChild(node, 1);
  u32 branch, branch_ref;

  Result result = CompileExpr(a, LinkNext, c);
  if (!result.ok) return result;

  branch = PushConst(IntVal(0), c->chunk);
  branch_ref = PushByte(OpBranch, c->chunk);

  PushByte(OpPop, c->chunk);
  result = CompileExpr(b, linkage, c);
  if (!result.ok) return result;

  PatchConst(c->chunk, branch, branch_ref);
  CompileLinkage(linkage, c);

  return CompileOk();
}

/* A ListNode starts with `nil`, then compiles each list item from back to
front, prepending each onto the result.

  nil
  ; for each item n:
  <code for num_items - n - 1>
  pair
*/
static Result CompileList(Node *node, bool linkage, Compiler *c)
{
  u32 i;
  PushConst(Nil, c->chunk);

  for (i = 0; i < NumNodeChildren(node); i++) {
    Node *item = NodeChild(node, NumNodeChildren(node) - i - 1);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushByte(OpPair, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

/* A TupleNode creates a tuple, then compiles each item and sets it.

  tuple
  ; for each item n:
  <code for n>
  const n
  set
*/
static Result CompileTuple(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node);

  PushConst(IntVal(num_items), c->chunk);
  PushByte(OpTuple, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *item = NodeChild(node, i);
    Result result = CompileExpr(item, LinkNext, c);
    if (!result.ok) return result;
    PushConst(IntVal(i), c->chunk);
    PushByte(OpSet, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

/* A MapNode creates an empty map, and for each entry compiles the value and
sets it in the map under its key.

  map
  ; for each key, value pair:
  <code for value>
  const <key>
  put
*/
static Result CompileMap(Node *node, bool linkage, Compiler *c)
{
  u32 i, num_items = NumNodeChildren(node) / 2;

  PushByte(OpMap, c->chunk);

  for (i = 0; i < num_items; i++) {
    Node *value = NodeChild(node, (i*2)+1);
    Node *key = NodeChild(node, i*2);

    Result result = CompileExpr(value, LinkNext, c);
    if (!result.ok) return result;

    result = CompileExpr(key, LinkNext, c);
    if (!result.ok) return result;

    PushByte(OpPut, c->chunk);
  }

  CompileLinkage(linkage, c);
  return CompileOk();
}

/* Strings are stored in the symbol table, and use a special opcode to fetch
their name.

  const <string symbol>
  str
*/
static Result CompileString(Node *node, bool linkage, Compiler *c)
{
  Val sym = NodeValue(node);
  Sym(SymbolName(sym, c->symbols), &c->chunk->symbols);
  PushConst(sym, c->chunk);
  PushByte(OpStr, c->chunk);
  CompileLinkage(linkage, c);
  return CompileOk();
}

/* Since the compiler keeps track of what symbols are defined and where in the
environment, the runtime only needs the variable's relative environment position
to find it.

  const <env position>
  lookup
*/
static Result CompileVar(Node *node, bool linkage, Compiler *c)
{
  Val id = NodeValue(node);
  i32 def = FrameFind(c->env, id);
  if (def == -1) return CompileError("Undefined variable", c);

  PushConst(IntVal(def), c->chunk);
  PushByte(OpLookup, c->chunk);

  CompileLinkage(linkage, c);
  return CompileOk();
}

/*
  const <value>
*/
static Result CompileConst(Node *node, bool linkage, Compiler *c)
{
  Val value = NodeValue(node);
  PushConst(value, c->chunk);
  CompileLinkage(linkage, c);
  if (IsSym(value)) {
    Sym(SymbolName(value, c->symbols), &c->chunk->symbols);
  }
  return CompileOk();
}

static void CompileLinkage(bool linkage, Compiler *c)
{
  if (linkage == LinkReturn) {
    PushByte(OpReturn, c->chunk);
  }
}

static Result CompileError(char *message, Compiler *c)
{
  return ErrorResult(message, c->filename, c->pos);
}

static void PushNodePos(Node *node, Compiler *c)
{
  PushSourcePos(node->pos - c->pos, c->chunk);
  c->pos = node->pos;
}

static u32 LambdaBegin(u32 num_params, Compiler *c)
{
  u32 lambda, lambda_ref, ref;
  lambda = PushConst(IntVal(0), c->chunk);
  PushConst(IntVal(num_params), c->chunk);
  lambda_ref = PushByte(OpLambda, c->chunk);
  PushConst(IntVal(0), c->chunk);
  ref = PushByte(OpJump, c->chunk);
  PatchConst(c->chunk, lambda, lambda_ref);
  return ref;
}

static void LambdaEnd(u32 ref, Compiler *c)
{
  PatchConst(c->chunk, ref - 2, ref);
}
