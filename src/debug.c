#include "debug.h"
#include "compile/lex.h"
#include "compile/project.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"
#include <stdio.h>

static void PrintSourceContext(u32 pos, char *source, u32 context);
static u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols);

static u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols)
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
  } else if (IsObj(value)) {
    if (mem && IsTuple(value, mem)) {
      return printf("t%d", RawVal(value));
    } else if (mem && IsBinary(value, mem)) {
      return printf("b%d", RawVal(value));
    } else if (mem && IsMap(value, mem)) {
      return printf("m%d", RawVal(value));
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
  u32 j, size = RawVal(VecRef(mem, index));
  u32 cells = NumBinCells(size);

  for (j = 0; j < cells; j++) {
    Val value = VecRef(mem, index+j+1);
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
  [OpDup]     = "dup",
  [OpConst]   = "const",
  [OpNeg]     = "neg",
  [OpNot]     = "not",
  [OpLen]     = "len",
  [OpMul]     = "mul",
  [OpDiv]     = "div",
  [OpRem]     = "rem",
  [OpAdd]     = "add",
  [OpSub]     = "sub",
  [OpIn]      = "in",
  [OpLt]      = "lt",
  [OpGt]      = "gt",
  [OpEq]      = "eq",
  [OpStr]     = "str",
  [OpPair]    = "pair",
  [OpUnpair]  = "unpair",
  [OpTuple]   = "tuple",
  [OpMap]     = "map",
  [OpSet]     = "set",
  [OpGet]     = "get",
  [OpCat]     = "cat",
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
    for (i = 0; i < TupleLength(frame, mem); i++) {
      Val item = TupleGet(frame, i, mem);
      u32 len = StrLen(SymbolName(item, symbols));
      if (written + len + 1 > 78) {
        for (j = written; j < 78; j++) printf(" ");
        printf("│\n");
        printf("│ ");
        written = 1;
      }
      written += PrintVal(TupleGet(frame, i, mem), mem, symbols);
      written += printf(" ");
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
    for (j = 0; j < OpLength(op) - 1; j++) {
      k += printf(" ");
      k += PrintVal(ChunkConst(chunk, i+j+1), 0, &chunk->symbols);
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
  for (i = 0; i < (i32)OpLength(op) - 1; i++) {
    Val arg = ChunkConst(vm->chunk, vm->pc + 1 + i);
    col_width -= printf(" ");

    col_width -= PrintVal(arg, &vm->mem, &vm->chunk->symbols);

    if (col_width < 0) break;
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
  Sym("*prim*", symbols);
  Sym("*func*", symbols);
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

  /* primitives */
  Sym("Kernel", symbols);
  Sym("Device", symbols);
  Sym("Type", symbols);
  Sym("head", symbols);
  Sym("tail", symbols);
  Sym("panic!", symbols);
  Sym("open", symbols);
  Sym("close", symbols);
  Sym("read", symbols);
  Sym("write", symbols);
  Sym("get-param", symbols);
  Sym("set-param", symbols);

  Sym("typeof", symbols);
  Sym("map-get", symbols);
  Sym("map-set", symbols);
  Sym("map-del", symbols);
  Sym("map-keys", symbols);
  Sym("split-bin", symbols);
  Sym("join-bin", symbols);
  Sym("trunc", symbols);
  Sym("symbol-name", symbols);
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

  printf("/* parse.h */\n");
  printf("#define SymEOF            0x%08X /* *eof* */\n", SymbolFor("*eof*"));
  printf("#define SymID             0x%08X /* *id* */\n", SymbolFor("*id*"));
  printf("#define SymBangEqual      0x%08X /* != */\n", SymbolFor("!="));
  printf("#define SymString         0x%08X /* \" */\n", SymbolFor("\""));
  printf("#define SymNewline        0x%08X /* *nl* */\n", SymbolFor("*nl*"));
  printf("#define SymHash           0x%08X /* # */\n", SymbolFor("#"));
  printf("#define SymPercent        0x%08X /* %% */\n", SymbolFor("%"));
  printf("#define SymAmpersand      0x%08X /* & */\n", SymbolFor("&"));
  printf("#define SymLParen         0x%08X /* ( */\n", SymbolFor("("));
  printf("#define SymRParen         0x%08X /* ) */\n", SymbolFor(")"));
  printf("#define SymStar           0x%08X /* * */\n", SymbolFor("*"));
  printf("#define SymPlus           0x%08X /* + */\n", SymbolFor("+"));
  printf("#define SymComma          0x%08X /* , */\n", SymbolFor(","));
  printf("#define SymMinus          0x%08X /* - */\n", SymbolFor("-"));
  printf("#define SymArrow          0x%08X /* -> */\n", SymbolFor("->"));
  printf("#define SymDot            0x%08X /* . */\n", SymbolFor("."));
  printf("#define SymSlash          0x%08X /* / */\n", SymbolFor("/"));
  printf("#define SymNum            0x%08X /* *num* */\n", SymbolFor("*num*"));
  printf("#define SymColon          0x%08X /* : */\n", SymbolFor(":"));
  printf("#define SymLess           0x%08X /* < */\n", SymbolFor("<"));
  printf("#define SymLessLess       0x%08X /* << */\n", SymbolFor("<<"));
  printf("#define SymLessEqual      0x%08X /* <= */\n", SymbolFor("<="));
  printf("#define SymLessGreater    0x%08X /* <> */\n", SymbolFor("<>"));
  printf("#define SymEqual          0x%08X /* = */\n", SymbolFor("="));
  printf("#define SymEqualEqual     0x%08X /* == */\n", SymbolFor("=="));
  printf("#define SymGreater        0x%08X /* > */\n", SymbolFor(">"));
  printf("#define SymGreaterEqual   0x%08X /* >= */\n", SymbolFor(">="));
  printf("#define SymGreaterGreater 0x%08X /* >> */\n", SymbolFor(">>"));
  printf("#define SymLBracket       0x%08X /* [ */\n", SymbolFor("["));
  printf("#define SymBackslash      0x%08X /* \\ */\n", SymbolFor("\\"));
  printf("#define SymRBracket       0x%08X /* ] */\n", SymbolFor("]"));
  printf("#define SymCaret          0x%08X /* ^ */\n", SymbolFor("^"));
  printf("#define SymAnd            0x%08X /* and */\n", SymbolFor("and"));
  printf("#define SymAs             0x%08X /* as */\n", SymbolFor("as"));
  printf("#define SymCond           0x%08X /* cond */\n", SymbolFor("cond"));
  printf("#define SymDef            0x%08X /* def */\n", SymbolFor("def"));
  printf("#define SymDo             0x%08X /* do */\n", SymbolFor("do"));
  printf("#define SymElse           0x%08X /* else */\n", SymbolFor("else"));
  printf("#define SymEnd            0x%08X /* end */\n", SymbolFor("end"));
  printf("#define SymFalse          0x%08X /* false */\n", SymbolFor("false"));
  printf("#define SymIf             0x%08X /* if */\n", SymbolFor("if"));
  printf("#define SymImport         0x%08X /* import */\n", SymbolFor("import"));
  printf("#define SymIn             0x%08X /* in */\n", SymbolFor("in"));
  printf("#define SymLet            0x%08X /* let */\n", SymbolFor("let"));
  printf("#define SymModule         0x%08X /* module */\n", SymbolFor("module"));
  printf("#define SymNil            0x%08X /* nil */\n", SymbolFor("nil"));
  printf("#define SymNot            0x%08X /* not */\n", SymbolFor("not"));
  printf("#define SymOr             0x%08X /* or */\n", SymbolFor("or"));
  printf("#define SymTrue           0x%08X /* true */\n", SymbolFor("true"));
  printf("#define SymLBrace         0x%08X /* { */\n", SymbolFor("{"));
  printf("#define SymBar            0x%08X /* | */\n", SymbolFor("|"));
  printf("#define SymRBrace         0x%08X /* } */\n", SymbolFor("}"));
  printf("#define SymTilde          0x%08X /* ~ */\n", SymbolFor("~"));

  printf("/* symbols.h */\n");
  printf("#define True              0x%08X /* true */\n", SymbolFor("true"));
  printf("#define False             0x%08X /* false */\n", SymbolFor("false"));
  printf("#define Ok                0x%08X /* ok */\n", SymbolFor("ok"));
  printf("#define Error             0x%08X /* error */\n", SymbolFor("error"));
  printf("#define Primitive         0x%08X /* *prim* */\n", SymbolFor("*prim*"));
  printf("#define Function          0x%08X /* *func* */\n", SymbolFor("*func*"));
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
  printf("  {/* head */         0x%08X, &VMHead},\n", SymbolFor("head"));
  printf("  {/* tail */         0x%08X, &VMTail},\n", SymbolFor("tail"));
  printf("  {/* panic! */       0x%08X, &VMPanic},\n", SymbolFor("panic!"));
  printf("  {/* unwrap */       0x%08X, &VMUnwrap},\n", SymbolFor("unwrap"));
  printf("  {/* unwrap! */      0x%08X, &VMForceUnwrap},\n", SymbolFor("unwrap!"));
  printf("  {/* ok? */          0x%08X, &VMOk},\n", SymbolFor("ok?"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveDef device[] = {\n");
  printf("  {/* open */         0x%08X, &VMOpen},\n", SymbolFor("open"));
  printf("  {/* close */        0x%08X, &VMClose},\n", SymbolFor("close"));
  printf("  {/* read */         0x%08X, &VMRead},\n", SymbolFor("read"));
  printf("  {/* write */        0x%08X, &VMWrite},\n", SymbolFor("write"));
  printf("  {/* get-param */    0x%08X, &VMGetParam},\n", SymbolFor("get-param"));
  printf("  {/* set-param */    0x%08X, &VMSetParam},\n", SymbolFor("set-param"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveDef type[] = {\n");
  printf("  {/* typeof */       0x%08X, &VMType},\n", SymbolFor("typeof"));
  printf("  {/* map-get */      0x%08X, &VMMapGet},\n", SymbolFor("map-get"));
  printf("  {/* map-set */      0x%08X, &VMMapSet},\n", SymbolFor("map-set"));
  printf("  {/* map-del */      0x%08X, &VMMapDelete},\n", SymbolFor("map-del"));
  printf("  {/* map-keys */     0x%08X, &VMMapKeys},\n", SymbolFor("map-keys"));
  printf("  {/* map-values */   0x%08X, &VMMapValues},\n", SymbolFor("map-values"));
  printf("  {/* split-bin */    0x%08X, &VMSplit},\n", SymbolFor("split-bin"));
  printf("  {/* join-bin */     0x%08X, &VMJoinBin},\n", SymbolFor("join-bin"));
  printf("  {/* trunc */        0x%08X, &VMTrunc},\n", SymbolFor("trunc"));
  printf("  {/* symbol-name */  0x%08X, &VMSymName},\n", SymbolFor("symbol-name"));
  printf("};\n");
  printf("\n");
  printf("static PrimitiveModuleDef primitives[] = {\n");
  printf("  {/* Kernel */       KernelMod, ArrayCount(kernel), kernel},\n");
  printf("  {/* Device */       0x%08X, ArrayCount(device), device},\n", SymbolFor("Device"));
  printf("  {/* Type */         0x%08X, ArrayCount(type), type},\n", SymbolFor("Type"));
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


static char *NodeTypeName(Val type)
{
  switch (type) {
  case SymID: return "ID";
  case SymBangEqual: return "BangEqual";
  case SymString: return "String";
  case SymHash: return "Hash";
  case SymPercent: return "Percent";
  case SymAmpersand: return "Ampersand";
  case SymLParen: return "Call";
  case SymStar: return "Star";
  case SymPlus: return "Plus";
  case SymMinus: return "Minus";
  case SymArrow: return "Lambda";
  case SymSlash: return "Slash";
  case SymNum: return "Num";
  case SymColon: return "Colon";
  case SymLess: return "Less";
  case SymLessLess: return "LessLess";
  case SymLessEqual: return "LessEqual";
  case SymLessGreater: return "LessGreater";
  case SymEqual: return "Equal";
  case SymEqualEqual: return "EqualEqual";
  case SymGreater: return "Greater";
  case SymGreaterEqual: return "GreaterEqual";
  case SymGreaterGreater: return "GreaterGreater";
  case SymLBracket: return "List";
  case SymCaret: return "Caret";
  case SymAnd: return "And";
  case SymDef: return "Def";
  case SymDo: return "Do";
  case SymIf: return "If";
  case SymImport: return "Import";
  case SymIn: return "In";
  case SymLet: return "Let";
  case SymNil: return "Nil";
  case SymNot: return "Not";
  case SymOr: return "Or";
  case SymLBrace: return "Tuple";
  case SymBar: return "Bar";
  case SymRBrace: return "Map";
  case SymTilde: return "Tilde";
  case SymModule: return "Module";
  default: return "<Unknown>";
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

static void PrintASTNode(Val node, u32 level, u32 lines, Parser *p)
{
  Val type = NodeType(node, p->mem);
  Val pos = NodePos(node, p->mem);
  Val expr = NodeExpr(node, p->mem);
  char *name = NodeTypeName(type);
  printf("%s:%d", name, RawInt(pos));

  if (type == SymDo) {
    Val assigns = Head(expr, p->mem);
    Val stmts = Tail(expr, p->mem);
    printf(" Assigns: [");
    while (assigns != Nil) {
      Val assign = Head(assigns, p->mem);
      printf("%s", SymbolName(assign, p->symbols));
      assigns = Tail(assigns, p->mem);
      if (assigns != Nil) printf(", ");
    }
    printf("]\n");

    lines |= Bit(level);
    while (stmts != Nil) {
      if (Tail(stmts, p->mem) == Nil) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(Head(stmts, p->mem), level+1, lines, p);
      stmts = Tail(stmts, p->mem);
    }
  } else if (type == SymLet || type == SymDef || type == SymModule) {
    printf(" %s\n", SymbolName(Head(expr, p->mem), p->symbols));
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(Tail(expr, p->mem), level+1, lines, p);
  } else if (type == SymImport) {
    printf(" %s as %s\n", SymbolName(Tail(expr, p->mem), p->symbols), SymbolName(Head(expr, p->mem), p->symbols));
  } else if (type == SymArrow) {
    Val params = Head(expr, p->mem);
    printf(" (");
    while (params != Nil) {
      printf("%s", SymbolName(Head(params, p->mem), p->symbols));
      params = Tail(params, p->mem);
      if (params != Nil) printf(", ");
    }
    printf(")\n");
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(Tail(expr, p->mem), level+1, lines, p);
  } else if (type == SymNum) {
    if (IsInt(expr)) {
      printf(" %d\n", RawInt(expr));
    } else if (IsFloat(expr)) {
      printf(" %f\n", RawFloat(expr));
    } else {
      printf("\n");
    }
  } else if (type == SymID) {
    printf(" %s\n", SymbolName(expr, p->symbols));
  } else if (type == SymColon) {
    if (expr == Nil) {
      printf(" :nil\n");
    } else {
      printf(" :%s\n", SymbolName(expr, p->symbols));
    }
  } else if (type == SymRBrace) {
    printf("\n");
    lines |= Bit(level);
    while (expr != Nil) {
      Val item = Head(expr, p->mem);
      if (Tail(expr, p->mem) == Nil) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      printf("%s: ", SymbolName(Head(item, p->mem), p->symbols));
      PrintASTNode(Tail(item, p->mem), level+1, lines, p);
      expr = Tail(expr, p->mem);
    }
  } else if (expr == Nil) {
    printf(" ()\n");
  } else {
    printf("\n");
    lines |= Bit(level);
    while (IsPair(expr) && expr != Nil) {
      if (Tail(expr, p->mem) == Nil) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(Head(expr, p->mem), level+1, lines, p);
      expr = Tail(expr, p->mem);
    }
  }
}

void PrintAST(Val module, Parser *p)
{
  Val name = ModuleName(module, p->mem);
  Val stmts = ModuleBody(module, p->mem);
  u32 lines = 1;

  printf("Module %s\n", SymbolName(name, p->symbols));
  while (stmts != Nil) {
    if (Tail(stmts, p->mem) == Nil) {
      lines = 0;
      printf("└╴");
    } else {
      printf("├╴");
    }

    PrintASTNode(Head(stmts, p->mem), 1, lines, p);
    stmts = Tail(stmts, p->mem);
  }
}
