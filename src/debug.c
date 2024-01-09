#include "debug.h"
#include "runtime/primitives.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include <stdio.h>

static u32 PrintInstruction(Chunk *chunk, u32 pos);

u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols)
{
  if (value == Nil) {
    return printf("nil");
  } else if (IsInt(value)) {
    return printf("%d", RawInt(value));
  } else if (IsFloat(value)) {
    return printf("%.2f", RawFloat(value));
  } else if (IsSym(value)) {
    if (symbols) {
      return printf("%s", SymbolName(value, symbols));
    } else {
      return printf("<%08X>", value);
    }
  } else if (IsPair(value)) {
    return printf("p%d", RawVal(value));
  } else if (IsPrimitive(value)) {
    PrimitiveDef *primitives = GetPrimitives();
    u32 fn = PrimNum(value);
    return printf("%%%s", primitives[fn].desc);
  } else if (IsObj(value)) {
    if (mem && IsTuple(value, mem)) {
      return printf("t%d", RawVal(value));
    } else if (mem && IsBinary(value, mem)) {
      if (BinaryCount(value, mem) <= 10) {
        u32 length = BinaryCount(value, mem);
        return printf("\"%*.*s\"", length, length, BinaryData(value, mem));
      }
      return printf("b%d", RawVal(value));
    } else if (mem && IsMap(value, mem)) {
      return printf("m%d", RawVal(value));
    } else if (mem && IsFunc(value, mem)) {
      return printf("f%d", RawVal(value));
    } else {
      return printf("o%d", RawVal(value));
    }
  } else {
    return printf("%08X", value);
  }
}

static void PrintCell(u32 index, Val value, u32 cell_width, SymbolTable *symbols);
static u32 PrintBinData(u32 index, u32 cell_width, u32 cols, Mem *mem);

void DumpMem(Mem *mem, SymbolTable *symbols)
{
  u32 i, j;
  u32 cols = 8;
  u32 cell_width = 10;

  printf("╔════╦");
  for (i = 0; i < cols-1; i++) {
    for (j = 0; j < cell_width; j++) printf("═");
    printf("╤");
  }
  for (j = 0; j < cell_width; j++) printf("═");
  printf("╗\n");

  for (i = 0; i < mem->count; i++) {
    if (i % cols == 0) printf("║%04d║", i);
    else printf("│");
    PrintCell(i, VecRef(mem, i), cell_width, symbols);
    if ((i+1) % cols == 0) printf("║\n");
    if (IsBinaryHeader(VecRef(mem, i))) {
      i += PrintBinData(i, cell_width, cols, mem);
    }
  }
  if (i % cols != 0) {
    while (i % cols != 0) {
      printf("│");
      for (j = 0; j < cell_width; j++) printf(" ");
      i++;
    }
    printf("║\n");
  }

  printf("╚════╩");
  for (i = 0; i < cols-1; i++) {
    for (j = 0; j < cell_width; j++) printf("═");
    printf("╧");
  }
  for (j = 0; j < cell_width; j++) printf("═");
  printf("╝\n");
}

static void PrintCell(u32 index, Val value, u32 cell_width, SymbolTable *symbols)
{
  if (IsInt(value)) {
    printf("%*d", cell_width, RawInt(value));
  } else if (IsFloat(value)) {
    printf("%*.1f", cell_width, RawFloat(value));
  } else if (IsSym(value) && symbols) {
    printf("%*.*s", cell_width, cell_width, SymbolName(value, symbols));
  } else if (value == Nil) {
    u32 i;
    for (i = 0; i < cell_width-3; i++) printf(" ");
    printf("nil");
  } else if (IsPair(value)) {
    printf("p%*d", cell_width-1, RawVal(value));
  } else if (IsObj(value)) {
    printf("o%*d", cell_width-1, RawVal(value));
  } else if (IsTupleHeader(value)) {
    printf("t%*d", cell_width-1, RawVal(value));
  } else if (IsBinaryHeader(value)) {
    printf("b%*d", cell_width-1, RawVal(value));
  } else if (IsMapHeader(value)) {
    printf("m%*X", cell_width-1, RawInt(value));
  } else {
    printf("  %08X", value);
  }
}

static u32 PrintBinData(u32 index, u32 cell_width, u32 cols, Mem *mem)
{
  u32 j, size = RawVal(MemRef(mem, index));
  u32 cells = NumBinCells(size);

  for (j = 0; j < cells; j++) {
    Val value = MemRef(mem, index+j+1);
    u32 bytes = (j == cells-1) ? (size-1) % 4 + 1 : 4;
    bool printable  = true;
    u32 k;

    for (k = 0; k < bytes; k++) {
      u8 byte = (value >> (k*8)) & 0xFF;
      if (!IsPrintable(byte)) {
        printable = false;
        break;
      }
    }

    if ((index+j+1) % cols == 0) printf("║%04d║", index+j+1);
    else printf("│");
    if (printable) {
      for (k = 0; k < cell_width-bytes-2; k++) printf(" ");
      printf("\"");
      for (k = 0; k < bytes; k++) printf("%c", (value >> (k*8)) & 0xFF);
      printf("\"");
    } else {
      for (k = 0; k < cell_width-(bytes*2); k++) printf(".");
      for (k = 0; k < bytes; k++) printf("%02X", (value >> (k*8)) & 0xFF);
    }
    if ((index+j+2) % cols == 0) printf("║\n");
  }
  return cells;
}

static void PrintASTNode(Node *node, u32 level, u32 lines, SymbolTable *symbols);
static char *NodeTypeName(NodeType type);
static void Indent(u32 level, u32 lines);

void PrintAST(Node *ast, SymbolTable *symbols)
{
  PrintASTNode(ast, 0, 0, symbols);
}

static void PrintASTNode(Node *node, u32 level, u32 lines, SymbolTable *symbols)
{
  NodeType type = node->type;
  char *name = NodeTypeName(type);
  u32 i;

  printf("%s:%d", name, node->pos);

  if (type == ModuleNode) {
    Val name = NodeValue(NodeChild(node, 3));
    Node *body = NodeChild(node, 2);

    printf(" %s\n", SymbolName(name, symbols));
    printf("└╴");
    PrintASTNode(body, level+1, lines, symbols);
  } else if (type == DoNode) {
    Node *assigns = NodeChild(node, 0);
    Node *stmts = NodeChild(node, 1);

    printf(" Assigns: [");
    for (i = 0; i < NumNodeChildren(assigns); i++) {
      Val assign = NodeValue(NodeChild(assigns, i));
      printf("%s", SymbolName(assign, symbols));
      if (i < NumNodeChildren(assigns) - 1) printf(", ");
    }
    printf("]\n");

    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(stmts); i++) {
      if (i == NumNodeChildren(stmts) - 1) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(NodeChild(stmts, i), level+1, lines, symbols);
    }
  } else if (type == LetNode || type == DefNode) {
    Val var = NodeValue(NodeChild(node, 0));
    printf(" %s\n", SymbolName(var, symbols));
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines, symbols);
  } else if (type == ImportNode) {
    Val mod = NodeValue(NodeChild(node, 0));
    Node *alias = NodeChild(node, 1);
    printf(" %s as ", SymbolName(mod, symbols));
    if (alias->type == NilNode) {
      printf("*\n");
    } else {
      printf("%s\n", SymbolName(NodeValue(alias), symbols));
    }
  } else if (type == LambdaNode) {
    Node *params = NodeChild(node, 0);
    printf(" (");
    for (i = 0; i < NumNodeChildren(params); i++) {
      Val param = NodeValue(NodeChild(params, i));
      printf("%s", SymbolName(param, symbols));
      if (i < NumNodeChildren(params) - 1) printf(", ");
    }
    printf(")\n");
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines, symbols);
  } else if (type == NumNode) {
    Val expr = NodeValue(node);
    if (IsInt(expr)) {
      printf(" %d\n", RawInt(expr));
    } else if (IsFloat(expr)) {
      printf(" %f\n", RawFloat(expr));
    } else {
      printf("\n");
    }
  } else if (type == IDNode) {
    printf(" %s\n", SymbolName(NodeValue(node), symbols));
  } else if (type == SymbolNode) {
    printf(" :%s\n", SymbolName(NodeValue(node), symbols));
  } else if (type == StringNode) {
    printf(" \"%s\"\n", SymbolName(NodeValue(node), symbols));
  } else if (type == MapNode) {
    printf("\n");
    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(node); i += 2) {
      Val key = NodeValue(NodeChild(node, i));
      Node *value = NodeChild(node, i+1);

      if (i == NumNodeChildren(node) - 2) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      printf("%s: ", SymbolName(key, symbols));
      PrintASTNode(value, level+1, lines, symbols);
    }
  } else if (type == NilNode) {
    printf("\n");
  } else {
    printf("\n");
    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(node); i++) {
      if (i == NumNodeChildren(node) - 1) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(NodeChild(node, i), level+1, lines, symbols);
    }
  }
}

static char *NodeTypeName(NodeType type)
{
  switch (type) {
  case ModuleNode: return "Module";
  case ImportNode: return "Import";
  case LetNode: return "Let";
  case DefNode: return "Def";
  case SymbolNode: return "Symbol";
  case DoNode: return "Do";
  case LambdaNode: return "Lambda";
  case CallNode: return "Call";
  case NilNode: return "Nil";
  case IfNode: return "If";
  case ListNode: return "List";
  case MapNode: return "Map";
  case TupleNode: return "Tuple";
  case IDNode: return "ID";
  case NumNode: return "Num";
  case StringNode: return "String";
  case AndNode: return "And";
  case OrNode: return "Or";
  }
}

static void Indent(u32 level, u32 lines)
{
  u32 i;
  for (i = 0; i < level; i++) {
    if (lines & Bit(i)) printf("│ ");
    else printf("  ");
  }
}

void PrintCompileEnv(Frame *frame, SymbolTable *symbols)
{
  /* primitive and variable names aren't usually saved in the symbol table.
     those will need to be saved before this prints anything useful. */
  while (frame != 0) {
    u32 i;
    printf("- ");
    for (i = 0; i < frame->count; i++) {
      PrintVal(frame->items[i], 0, symbols);
      printf(" ");
    }
    printf("\n");
    frame = frame->parent;
  }
}

void Disassemble(Chunk *chunk)
{
  u32 col1 = 7, col2, col3, col4, width = 80;
  u32 i, line;
  char *sym;
  char *filename = ChunkFileAt(0, chunk);
  u32 next_file = ChunkFileByteSize(0, chunk);

  col2 = col1 + Max(5, NumDigits(chunk->code.count, 10) + 1);
  col3 = col2 + 24;
  col4 = col3 + 24;

  /* header */
  printf("╔ Disassembled ");
  i = 15;
  while (i++ < width - 1) printf("═");
  printf("╗\n");

  printf("║  src  idx");
  i = 11;
  while (i < col2) i += printf(" ");
  i += printf(" Instruction");
  while (i < col3) i += printf(" ");
  i += printf("Constants");
  while (i < col4) i += printf(" ");
  i += printf("Symbols");
  while (i < width - 1) i += printf(" ");
  printf("║\n");

  printf("╟─────┬");
  for (i = 7; i < col2 - 1; i++) printf("─");
  printf("┬");
  for (i = 0; i < width - col2 - 1; i++) printf("─");
  printf("╢\n");

  /* on each line, print the instruction, the next constant, and the next symbol */
  line = 0;
  sym = (char*)chunk->symbols.names.items;
  for (i = 0; i < chunk->code.count; i += OpLength(ChunkRef(chunk, i))) {
    u32 j, k;
    u32 source = SourcePosAt(i, chunk);

    /* print file boundary when the file changes */
    if (i == next_file) {
      u32 written;
      filename = ChunkFileAt(i, chunk);
      next_file += ChunkFileByteSize(i, chunk);
      printf("╟─");
      written = printf("%s", filename);
      for (j = written+2; j < width-1; j++) printf("─");
      printf("╢\n");
    }

    printf("║");

    /* source position and byte index */
    printf("%5d│%*d│", source, col2 - col1 - 1, i);
    k = col2;

    k += PrintInstruction(chunk, i);
    while (k < col3) k += printf(" ");

    /* print next constant */
    if (line < chunk->constants.count) {
      Val value = chunk->constants.items[line];
      k += printf("%3d: ", line);
      if (IsSym(value)) {
        k += printf("%.*s", col4-col3, SymbolName(value, &chunk->symbols));
      } else {
        k += PrintVal(chunk->constants.items[line], 0, &chunk->symbols);
      }
    }
    while (k < col4) k += printf(" ");

    /* print next symbol */
    if ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
      u32 length = StrLen(sym);
      k += printf("%.*s", width-col4-1, sym);
      sym += length + 1;
    }
    while (k < width - 1) k += printf(" ");

    printf("║\n");
    line++;
  }

  /* print the rest of the symbols */
  while ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
    u32 length = StrLen(sym);
    printf("║");
    for (i = 1; i < col4; i++) printf(" ");

    i += printf("%.*s", width-col4-1, sym);
    sym += length + 1;

    while (i < width - 1) i += printf(" ");
    printf("║\n");
  }

  /* print footer */
  printf("╚");
  for (i = 0; i < width-2; i++) printf("═");
  printf("╝\n");
}

void DumpSourceMap(Chunk *chunk)
{
  ByteVec map = chunk->source_map;
  u32 i, j, pos = 0, bytes = 0;
  char *file;
  char *source;
  u32 next_file = 0;
  Lexer lex;

  for (i = 0; i < map.count; i += 2) {
    if (bytes == 0 || bytes >= next_file) {
      file = ChunkFileAt(bytes, chunk);
      source = ReadFile(file);
      next_file += ChunkFileByteSize(bytes, chunk);
      printf("%s (%d)\n", file, next_file);
    }

    if ((i8)map.items[i] == -128) {
      pos = 0;
    } else {
      pos += (i8)map.items[i];
    }

    if (source) {
      u32 line = LineNum(source, pos);
      u32 col = ColNum(source, pos);
      InitLexer(&lex, source, pos);
      printf("[%d:%d] %.*s\n", line + 1, col + 1, lex.token.length, lex.token.lexeme);
    }

    for (j = bytes; j < bytes + map.items[i+1]; j += OpLength(ChunkRef(chunk, j))) {
      printf("  ");
      PrintInstruction(chunk, j);
      printf("\n");
    }

    bytes += map.items[i+1];
  }
}

void PrintTraceHeader(u32 chunk_size)
{
  u32 col_size = Max(4, NumDigits(chunk_size, 10));
  u32 i;
  printf(" PC ");
  for (i = 0; i < col_size - 4; i++) printf(" ");
  printf("  Instruction           Env   Stack\n");
  for (i = 0; i < col_size; i++) printf("─");
  printf("┬─────────────────────┬─────┬─────────────────\n");
}

void TraceInstruction(VM *vm)
{
  i32 i, col_width;
  u32 col1 = Max(4, NumDigits(vm->chunk->code.count, 10));

  printf("%*d│ ", col1, vm->pc);

  col_width = 20;
  col_width -= PrintInstruction(vm->chunk, vm->pc);
  for (i = 0; i < col_width; i++) printf(" ");
  printf("│");

  if (vm->stack.count == 0 || Env(vm) == Nil) {
    printf("     ");
  } else {
    printf("%5d", RawVal(Env(vm)));
  }
  printf("│");

  col_width = 80;
  if (vm->stack.count > 0) {
    for (i = 0; i < (i32)vm->stack.count-1; i++) {
      col_width -= printf(" ");
      col_width -= PrintVal(StackRef(vm, i), &vm->mem, &vm->chunk->symbols);
      if (col_width < 0) {
        printf(" ...");
        break;
      }
    }
  }
  printf("\n");
}

void PrintEnv(Val env, Mem *mem, SymbolTable *symbols)
{
  u32 i, j;
  env = ReverseList(env, Nil, mem);

  printf("┌ Environment ");
  for (i = 0; i < 65; i++) printf("─");
  printf("┐\n");

  while (env != Nil) {
    Val frame = Head(env, mem);
    u32 written = 1;
    bool is_last = Tail(env, mem) == Nil;

    printf("│ ");
    for (i = 0; i < TupleCount(frame, mem); i++) {
      written += PrintVal(TupleGet(frame, i, mem), mem, symbols);
      written += printf(" ");
      if (written > 78) {
        printf("│\n");
        printf("│ ");
        written = 1;
      }
    }
    for (j = written; j < 78; j++) printf(" ");
    printf("│\n");
    if (!is_last) printf("├");
    else printf("└");
    for (i = 0; i < 78; i++) printf("─");
    if (!is_last) printf("┤\n");
    else printf("┘\n");

    env = Tail(env, mem);
  }
}

static u32 PrintInstruction(Chunk *chunk, u32 pos)
{
  u8 op;
  u32 printed = 0;

  if (pos >= chunk->code.count) return 0;

  op = ChunkRef(chunk, pos);

  printed += printf(" %s", OpName(op));
  if (op == OpInt || op == OpApply) {
    printed += printf(" %d", ChunkRef(chunk, pos+1));
  } else if (op == OpConst2) {
    u32 index = (ChunkRef(chunk, pos+1) << 8) | ChunkRef(chunk, pos+2);
    Val arg = chunk->constants.items[index];
    printed += printf(" %d ", index);
    printed += PrintVal(arg, 0, &chunk->symbols);
  } else {
    u32 j;
    for (j = 0; j < OpLength(op) - 1; j++) {
      printed += printf(" ");
      printed += PrintVal(ChunkConst(chunk, pos+j+1), 0, &chunk->symbols);
    }
  }
  return printed;
}
