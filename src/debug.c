#include "debug.h"
#include "compile/lex.h"
#include "compile/project.h"
#include "runtime/primitives.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"
#include <stdio.h>

static void PrintSourceContext(u32 pos, char *source, u32 context);

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
    return printf("λ%s", primitives[fn].desc) - 1;
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

static char *op_names[] = {
  [OpNoop]    = "noop",
  [OpHalt]    = "halt",
  [OpError]   = "error",
  [OpPop]     = "pop",
  [OpSwap]    = "swap",
  [OpDup]     = "dup",
  [OpConst]   = "const",
  [OpConst2]  = "const2",
  [OpInt]     = "int",
  [OpNil]     = "nil",
  [OpStr]     = "str",
  [OpPair]    = "pair",
  [OpUnpair]  = "unpair",
  [OpTuple]   = "tuple",
  [OpMap]     = "map",
  [OpSet]     = "set",
  [OpGet]     = "get",
  [OpExtend]  = "extend",
  [OpDefine]  = "define",
  [OpLookup]  = "lookup",
  [OpExport]  = "export",
  [OpJump]    = "jump",
  [OpBranch]  = "branch",
  [OpLambda]  = "lambda",
  [OpLink]    = "link",
  [OpReturn]  = "return",
  [OpApply]   = "apply"
};

static char *OpName(OpCode op)
{
  return op_names[op];
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

void PrintCompileEnv(Frame *frame, SymbolTable *symbols)
{
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
  u32 longest_sym, width, col1, col2;
  u32 i, line;
  char *sym;
  char *filename = ChunkFile(0, chunk);
  u32 next_file = ChunkFileLength(0, chunk);

  longest_sym = Min(17, LongestSymbol(&chunk->symbols));
  col1 = 20 + longest_sym;
  col2 = col1 + 6 + longest_sym;
  width = col2 + longest_sym + 2;

  printf("╔ Disassembled ");
  for (i = 0; i < width-16; i++) printf("═");
  printf("╗\n");

  printf("║  src  idx  Instruction");
  for (i = 24; i < col1; i++) printf(" ");
  i += printf("Constants");
  while (i < col2) i += printf(" ");
  i += printf("Symbols");
  while (i < width - 1) i += printf(" ");
  printf("║\n");

  printf("╟─────┬────┬");
  for (i = 12; i < width-1; i++) printf("─");
  printf("╢\n");

  line = 0;
  sym = (char*)chunk->symbols.names.items;
  for (i = 0; i < chunk->code.count; i += OpLength(ChunkRef(chunk, i))) {
    u8 op = ChunkRef(chunk, i);
    u32 j, k;
    u32 source = GetSourcePosition(i, chunk);

    if (i == next_file) {
      u32 written;
      filename = ChunkFile(i, chunk);
      next_file += ChunkFileLength(i, chunk);
      printf("╟─");
      written = printf("%s", filename);
      for (j = written+2; j < width-1; j++) printf("─");
      printf("╢\n");
    }

    printf("║");
    printf(" %4d│%4d│", source, i);
    k = 12;
    k += printf(" %s", OpName(op));
    if (op == OpInt || op == OpApply) {
      k += printf(" %d", ChunkRef(chunk, i+1));
    } else if (op == OpConst2) {
      u32 index = (ChunkRef(chunk, i+1) << 8) | ChunkRef(chunk, i+2);
      Val arg = chunk->constants[index];
      k += printf(" %d ", index);
      k += PrintVal(arg, 0, &chunk->symbols);
    } else {
      for (j = 0; j < OpLength(op) - 1; j++) {
        k += printf(" ");
        k += PrintVal(ChunkConst(chunk, i+j+1), 0, &chunk->symbols);
      }
    }

    while (k < col1) k += printf(" ");
    if (line < chunk->num_constants) {
      Val value = chunk->constants[line];
      k += printf("%3d: ", line);
      if (IsSym(value)) {
        k += printf("%.*s", longest_sym, SymbolName(value, &chunk->symbols));
      } else {
        k += PrintVal(chunk->constants[line], 0, &chunk->symbols);
      }
    }

    while (k < col2) k += printf(" ");
    if ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
      u32 length = StrLen(sym);
      k += printf("%.*s", longest_sym, sym);
      sym += length + 1;
    }

    while (k < width - 1) k += printf(" ");
    printf("║\n");
    line++;
  }

  while ((u8*)sym < chunk->symbols.names.items + chunk->symbols.names.count) {
    u32 length = StrLen(sym);
    printf("║");
    i = 1;
    while (i < col2) i += printf(" ");
    i += printf("%.*s", longest_sym, sym);
    sym += length + 1;

    while (i < width - 1) i += printf(" ");
    printf("║\n");
  }

  printf("╚");
  for (i = 0; i < width-2; i++) printf("═");
  printf("╝\n");
}

void PrintTraceHeader(void)
{
  printf(" PC   Instruction           Env   Stack\n");
  printf("────┬─────────────────────┬─────┬─────────────────\n");
}

void TraceInstruction(OpCode op, VM *vm)
{
  i32 i, col_width;

  printf("%4d│ ", vm->pc);

  col_width = 20;
  col_width -= printf("%s", OpName(op));
  if (op == OpInt || op == OpApply) {
    col_width -= printf(" %d", ChunkRef(vm->chunk, vm->pc+1));
  } else if (op == OpConst2) {
    u32 index = (ChunkRef(vm->chunk, vm->pc+1) << 8) | ChunkRef(vm->chunk, vm->pc+2);
    Val arg = vm->chunk->constants[index];
    col_width -= printf(" ");
    col_width -= PrintVal(arg, &vm->mem, &vm->chunk->symbols);
  } else {
    for (i = 0; i < (i32)OpLength(op) - 1; i++) {
      Val arg = ChunkConst(vm->chunk, vm->pc + 1 + i);
      col_width -= printf(" ");

      col_width -= PrintVal(arg, &vm->mem, &vm->chunk->symbols);

      if (col_width < 0) break;
    }
  }
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

void DefineSymbols(SymbolTable *symbols)
{
  Sym("return", symbols);
  Sym("next", symbols);
  Sym("*eof*", symbols);
  Sym("*id*", symbols);
  Sym("!=", symbols);
  Sym("\"", symbols);
  Sym("*nl*", symbols);
  Sym("#", symbols);
  Sym("%", symbols);
  Sym("&", symbols);
  Sym("(", symbols);
  Sym(")", symbols);
  Sym("*", symbols);
  Sym("+", symbols);
  Sym(",", symbols);
  Sym("-", symbols);
  Sym("->", symbols);
  Sym(".", symbols);
  Sym("/", symbols);
  Sym("*num*", symbols);
  Sym(":", symbols);
  Sym("<", symbols);
  Sym("<<", symbols);
  Sym("<=", symbols);
  Sym("<>", symbols);
  Sym("=", symbols);
  Sym("==", symbols);
  Sym(">", symbols);
  Sym(">=", symbols);
  Sym(">>", symbols);
  Sym("[", symbols);
  Sym("]", symbols);
  Sym("^", symbols);
  Sym("and", symbols);
  Sym("as", symbols);
  Sym("cond", symbols);
  Sym("def", symbols);
  Sym("do", symbols);
  Sym("else", symbols);
  Sym("end", symbols);
  Sym("false", symbols);
  Sym("if", symbols);
  Sym("import", symbols);
  Sym("in", symbols);
  Sym("let", symbols);
  Sym("module", symbols);
  Sym("nil", symbols);
  Sym("not", symbols);
  Sym("or", symbols);
  Sym("true", symbols);
  Sym("{", symbols);
  Sym("|", symbols);
  Sym("}", symbols);
  Sym("~", symbols);
  Sym("true", symbols);
  Sym("false", symbols);
  Sym("ok", symbols);
  Sym("error", symbols);
  Sym("*undefined*", symbols);
  Sym("*moved*", symbols);
  Sym("*file*", symbols);
  Sym("float", symbols);
  Sym("integer", symbols);
  Sym("number", symbols);
  Sym("symbol", symbols);
  Sym("pair", symbols);
  Sym("tuple", symbols);
  Sym("binary", symbols);
  Sym("map", symbols);
  DefinePrimitiveSymbols(symbols);
}

void DefinePrimitiveSymbols(SymbolTable *symbols)
{
  u32 i;
  PrimitiveDef *primitives = GetPrimitives();
  for (i = 0; i < NumPrimitives(); i++) {
    Sym(primitives[i].desc, symbols);
  }
}

/*
Since generating symbols takes some time, all of the symbols used by the
compiler and VM are precomputed and defined in source files. This function is
used to regenerate them.
*/
void GenerateSymbols(void)
{
  printf("/* compile.c */\n");
  printf("#define LinkReturn        0x%08X /* return */\n", SymbolFor("return"));
  printf("#define LinkNext          0x%08X /* next */\n", SymbolFor("next"));

  printf("/* parse.c */\n");
  printf("static Val OpSymbol(TokenType token_type)\n");
  printf("{\n");
  printf("  switch (token_type) {\n");
  printf("  case TokenBangEq:     return 0x%08X;\n", SymbolFor("!="));
  printf("  case TokenHash:       return 0x%08X;\n", SymbolFor("#"));
  printf("  case TokenPercent:    return 0x%08X;\n", SymbolFor("%"));
  printf("  case TokenAmpersand:  return 0x%08X;\n", SymbolFor("&"));
  printf("  case TokenStar:       return 0x%08X;\n", SymbolFor("*"));
  printf("  case TokenPlus:       return 0x%08X;\n", SymbolFor("+"));
  printf("  case TokenMinus:      return 0x%08X;\n", SymbolFor("-"));
  printf("  case TokenDot:        return 0x%08X;\n", SymbolFor("."));
  printf("  case TokenSlash:      return 0x%08X;\n", SymbolFor("/"));
  printf("  case TokenLt:         return 0x%08X;\n", SymbolFor("<"));
  printf("  case TokenLtLt:       return 0x%08X;\n", SymbolFor("<<"));
  printf("  case TokenLtEq:       return 0x%08X;\n", SymbolFor("<="));
  printf("  case TokenLtGt:       return 0x%08X;\n", SymbolFor("<>"));
  printf("  case TokenEqEq:       return 0x%08X;\n", SymbolFor("=="));
  printf("  case TokenGt:         return 0x%08X;\n", SymbolFor(">"));
  printf("  case TokenGtEq:       return 0x%08X;\n", SymbolFor(">="));
  printf("  case TokenGtGt:       return 0x%08X;\n", SymbolFor(">>"));
  printf("  case TokenCaret:      return 0x%08X;\n", SymbolFor("^"));
  printf("  case TokenIn:         return 0x%08X;\n", SymbolFor("in"));
  printf("  case TokenNot:        return 0x%08X;\n", SymbolFor("not"));
  printf("  case TokenBar:        return 0x%08X;\n", SymbolFor("|"));
  printf("  case TokenTilde:      return 0x%08X;\n", SymbolFor("~"));
  printf("  default:              return Nil;\n");
  printf("  }\n");
  printf("}\n");

  printf("/* symbols.h */\n");
  printf("#define True              0x%08X /* true */\n", SymbolFor("true"));
  printf("#define False             0x%08X /* false */\n", SymbolFor("false"));
  printf("#define Ok                0x%08X /* ok */\n", SymbolFor("ok"));
  printf("#define Error             0x%08X /* error */\n", SymbolFor("error"));
  printf("#define Undefined         0x%08X /* *undefined* */\n", SymbolFor("*undefined*"));
  printf("#define Moved             0x%08X /* *moved* */\n", SymbolFor("*moved*"));
  printf("#define File              0x%08X /* *file* */\n", SymbolFor("*file*"));
  printf("#define FloatType         0x%08X /* float */\n", SymbolFor("float"));
  printf("#define IntType           0x%08X /* integer */\n", SymbolFor("integer"));
  printf("#define NumType           0x%08X /* number */\n", SymbolFor("number"));
  printf("#define SymType           0x%08X /* symbol */\n", SymbolFor("symbol"));
  printf("#define PairType          0x%08X /* pair */\n", SymbolFor("pair"));
  printf("#define TupleType         0x%08X /* tuple */\n", SymbolFor("tuple"));
  printf("#define BinaryType        0x%08X /* binary */\n", SymbolFor("binary"));
  printf("#define MapType           0x%08X /* map */\n", SymbolFor("map"));
  printf("#define AnyType           0x%08X /* any */\n", SymbolFor("any"));

  printf("/* primitives.h */\n");
  printf("#define KernelMod         0x%08X /* Kernel */\n", SymbolFor("Kernel"));

  printf("/* device.c */\n");
  printf("#define SymConsole        0x%08X /* console */\n", SymbolFor("console"));
  printf("#define SymFile           0x%08X /* file */\n", SymbolFor("file"));
  printf("#define SymDirectory      0x%08X /* directory */\n", SymbolFor("directory"));
  printf("#define SymSerial         0x%08X /* serial */\n", SymbolFor("serial"));
  printf("#define SymSystem         0x%08X /* system */\n", SymbolFor("system"));
  printf("#define SymWindow         0x%08X /* window */\n", SymbolFor("window"));

  printf("/* directory.c */\n");
  printf("#define SymDevice         0x%08X /* device */\n", SymbolFor("device"));
  printf("#define SymDirectory      0x%08X /* directory */\n", SymbolFor("directory"));
  printf("#define SymPipe           0x%08X /* pipe */\n", SymbolFor("pipe"));
  printf("#define SymLink           0x%08X /* link */\n", SymbolFor("link"));
  printf("#define SymFile           0x%08X /* file */\n", SymbolFor("file"));
  printf("#define SymSocket         0x%08X /* socket */\n", SymbolFor("socket"));
  printf("#define SymUnknown        0x%08X /* unknown */\n", SymbolFor("unknown"));
  printf("#define SymPath           0x%08X /* path */\n", SymbolFor("path"));

  printf("/* file.c */\n");
  printf("#define SymPosition       0x%08X /* position */\n", SymbolFor("position"));

  printf("/* system.c */\n");
  printf("#define SymSeed           0x%08X /* seed */\n", SymbolFor("seed"));
  printf("#define SymRandom         0x%08X /* random */\n", SymbolFor("random"));
  printf("#define SymTime           0x%08X /* time */\n", SymbolFor("time"));

  printf("/* window.c */\n");
  printf("#define SymClear          0x%08X /* clear */\n", SymbolFor("clear"));
  printf("#define SymText           0x%08X /* text */\n", SymbolFor("text"));
  printf("#define SymLine           0x%08X /* line */\n", SymbolFor("line"));
  printf("#define SymBlit           0x%08X /* blit */\n", SymbolFor("blit"));
  printf("#define SymWidth          0x%08X /* width */\n", SymbolFor("width"));
  printf("#define SymHeight         0x%08X /* height */\n", SymbolFor("height"));
  printf("#define SymFont           0x%08X /* font */\n", SymbolFor("font"));
  printf("#define SymFontSize       0x%08X /* fontsize */\n", SymbolFor("font-size"));
  printf("#define SymColor          0x%08X /* color */\n", SymbolFor("color"));
}

void GeneratePrimitives(void)
{
  printf("static PrimitiveDef kernel[] = {\n");
  printf("  {\"head\",            0x%08X, &VMHead},\n", SymbolFor("head"));
  printf("  {\"tail\",            0x%08X, &VMTail},\n", SymbolFor("tail"));
  printf("  {\"panic!\",          0x%08X, &VMPanic},\n", SymbolFor("panic!"));
  printf("  {\"unwrap\",          0x%08X, &VMUnwrap},\n", SymbolFor("unwrap"));
  printf("  {\"unwrap!\",         0x%08X, &VMForceUnwrap},\n", SymbolFor("unwrap!"));
  printf("  {\"ok?\",             0x%08X, &VMOk},\n", SymbolFor("ok?"));
  printf("  {\"float?\",          0x%08X, &VMIsFloat},\n", SymbolFor("float?"));
  printf("  {\"integer?\",        0x%08X, &VMIsInt},\n", SymbolFor("integer?"));
  printf("  {\"symbol?\",         0x%08X, &VMIsSym},\n", SymbolFor("symbol?"));
  printf("  {\"pair?\",           0x%08X, &VMIsPair},\n", SymbolFor("pair?"));
  printf("  {\"tuple?\",          0x%08X, &VMIsTuple},\n", SymbolFor("tuple?"));
  printf("  {\"binary?\",         0x%08X, &VMIsBin},\n", SymbolFor("binary?"));
  printf("  {\"map?\",            0x%08X, &VMIsMap},\n", SymbolFor("map?"));
  printf("  {\"function?\",       0x%08X, &VMIsFunc},\n", SymbolFor("function?"));
  printf("  {\"!=\",              0x%08X, &VMNotEqual},\n", SymbolFor("!="));
  printf("  {\"#\",               0x%08X, &VMLen},\n", SymbolFor("#"));
  printf("  {\"%%\",               0x%08X, &VMRem},\n", SymbolFor("%"));
  printf("  {\"&\",               0x%08X, &VMBitAnd},\n", SymbolFor("&"));
  printf("  {\"*\",               0x%08X, &VMMul},\n", SymbolFor("*"));
  printf("  {\"+\",               0x%08X, &VMAdd},\n", SymbolFor("+"));
  printf("  {\"-\",               0x%08X, &VMSub},\n", SymbolFor("-"));
  printf("  {\"/\",               0x%08X, &VMDiv},\n", SymbolFor("/"));
  printf("  {\"<\",               0x%08X, &VMLess},\n", SymbolFor("<"));
  printf("  {\"<<\",              0x%08X, &VMShiftLeft},\n", SymbolFor("<<"));
  printf("  {\"<=\",              0x%08X, &VMLessEqual},\n", SymbolFor("<="));
  printf("  {\"<>\",              0x%08X, &VMCat},\n", SymbolFor("<>"));
  printf("  {\"==\",              0x%08X, &VMEqual},\n", SymbolFor("=="));
  printf("  {\">\",               0x%08X, &VMGreater},\n", SymbolFor(">"));
  printf("  {\">=\",              0x%08X, &VMGreaterEqual},\n", SymbolFor(">="));
  printf("  {\">>\",              0x%08X, &VMShiftRight},\n", SymbolFor(">>"));
  printf("  {\"^\",               0x%08X, &VMBitOr},\n", SymbolFor("^"));
  printf("  {\"in\",              0x%08X, &VMIn},\n", SymbolFor("in"));
  printf("  {\"not\",             0x%08X, &VMNot},\n", SymbolFor("not"));
  printf("  {\"|\",               0x%08X, &VMPair},\n", SymbolFor("|"));
  printf("  {\"~\",               0x%08X, &VMBitNot},\n", SymbolFor("~"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveDef device[] = {\n");
  printf("  {\"open\",            0x%08X, &VMOpen},\n", SymbolFor("open"));
  printf("  {\"close\",           0x%08X, &VMClose},\n", SymbolFor("close"));
  printf("  {\"read\",            0x%08X, &VMRead},\n", SymbolFor("read"));
  printf("  {\"write\",           0x%08X, &VMWrite},\n", SymbolFor("write"));
  printf("  {\"get-param\",       0x%08X, &VMGetParam},\n", SymbolFor("get-param"));
  printf("  {\"set-param\",       0x%08X, &VMSetParam},\n", SymbolFor("set-param"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveDef type[] = {\n");
  printf("  {\"map-get\",         0x%08X, &VMMapGet},\n", SymbolFor("map-get"));
  printf("  {\"map-set\",         0x%08X, &VMMapSet},\n", SymbolFor("map-set"));
  printf("  {\"map-del\",         0x%08X, &VMMapDelete},\n", SymbolFor("map-del"));
  printf("  {\"map-keys\",        0x%08X, &VMMapKeys},\n", SymbolFor("map-keys"));
  printf("  {\"map-values\",      0x%08X, &VMMapValues},\n", SymbolFor("map-values"));
  printf("  {\"split-bin\",       0x%08X, &VMSplit},\n", SymbolFor("split-bin"));
  printf("  {\"join-bin\",        0x%08X, &VMJoinBin},\n", SymbolFor("join-bin"));
  printf("  {\"stuff\",           0x%08X, &VMStuff},\n", SymbolFor("stuff"));
  printf("  {\"trunc\",           0x%08X, &VMTrunc},\n", SymbolFor("trunc"));
  printf("  {\"symbol-name\",     0x%08X, &VMSymName},\n", SymbolFor("symbol-name"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveModuleDef primitives[] = {\n");

  printf("{\"Kernel\",            0x%08X, ArrayCount(kernel), kernel},\n", SymbolFor("Kernel"));
  printf("{\"Device\",            0x%08X, ArrayCount(device), device},\n", SymbolFor("Device"));
  printf("{\"Type\",              0x%08X, ArrayCount(type), type},\n", SymbolFor("Type"));
  printf("};\n");
}

void PrintMemory(u32 amount)
{
  char *suffixes[] = {"B", "kB", "MB", "GB", "TB"};
  u32 i;
  u32 frac = 0;
  for (i = 0; i < ArrayCount(suffixes); i++) {
    if (amount <= 1024) break;
    frac = amount % 1024;
    amount /= 1024;
  }
  if (frac == 0) {
    printf("%d%s", amount, suffixes[i]);
  } else {
    printf("%d.%d%s", amount, frac, suffixes[i]);
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

static void PrintASTNode(Node *node, u32 level, u32 lines, Parser *p)
{
  NodeType type = node->type;
  char *name = NodeTypeName(type);
  u32 i;

  printf("%s:%d", name, node->pos);

  if (type == ModuleNode) {
    Val name = NodeValue(NodeChild(node, 3));
    Node *body = NodeChild(node, 2);

    printf(" %s\n", SymbolName(name, p->symbols));
    printf("└╴");
    PrintASTNode(body, level+1, lines, p);
  } else if (type == DoNode) {
    Node *assigns = NodeChild(node, 0);
    Node *stmts = NodeChild(node, 1);

    printf(" Assigns: [");
    for (i = 0; i < NumNodeChildren(assigns); i++) {
      Val assign = NodeValue(NodeChild(assigns, i));
      printf("%s", SymbolName(assign, p->symbols));
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

      PrintASTNode(NodeChild(stmts, i), level+1, lines, p);
    }
  } else if (type == LetNode || type == DefNode) {
    Val var = NodeValue(NodeChild(node, 0));
    printf(" %s\n", SymbolName(var, p->symbols));
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines, p);
  } else if (type == ImportNode) {
    Val mod = NodeValue(NodeChild(node, 0));
    Node *alias = NodeChild(node, 1);
    printf(" %s as ", SymbolName(mod, p->symbols));
    if (alias->type == NilNode) {
      printf("*\n");
    } else {
      printf("%s\n", SymbolName(NodeValue(alias), p->symbols));
    }
  } else if (type == LambdaNode) {
    Node *params = NodeChild(node, 0);
    printf(" (");
    for (i = 0; i < NumNodeChildren(params); i++) {
      Val param = NodeValue(NodeChild(params, i));
      printf("%s", SymbolName(param, p->symbols));
      if (i < NumNodeChildren(params) - 1) printf(", ");
    }
    printf(")\n");
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines, p);
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
    printf(" %s\n", SymbolName(NodeValue(node), p->symbols));
  } else if (type == SymbolNode) {
    printf(" :%s\n", SymbolName(NodeValue(node), p->symbols));
  } else if (type == StringNode) {
    printf(" \"%s\"\n", SymbolName(NodeValue(node), p->symbols));
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

      printf("%s: ", SymbolName(key, p->symbols));
      PrintASTNode(value, level+1, lines, p);
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

      PrintASTNode(NodeChild(node, i), level+1, lines, p);
    }
  }
}

void PrintAST(Node *ast, Parser *p)
{
  PrintASTNode(ast, 0, 0, p);
}
