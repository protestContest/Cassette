#ifdef DEBUG

#include "debug.h"
#include "compile/lex.h"
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
    } else if (IsBignum(value, mem)) {
      i64 num = ((u64*)(mem->values + RawVal(value) + 1))[0];
      return printf("%lld", num);
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
    printf("@%*d", cell_width-1, RawVal(value));
  } else if (IsBinaryHeader(value)) {
    printf("*%*d", cell_width-1, RawVal(value));
  } else if (IsBignumHeader(value)) {
    printf("#%*d", cell_width-1, RawInt(value));
  } else {
    printf("%0*.*X", (cell_width+1)/2, cell_width/2, value);
  }
}

static u32 PrintBinData(u32 index, u32 cell_width, u32 cols, Mem *mem)
{
  u32 j, size = RawVal(mem->values[index]);
  u32 cells = NumBinCells(size);

  for (j = 0; j < cells; j++) {
    Val value = mem->values[index+j+1];
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
    PrintCell(i, mem->values[i], cell_width, symbols);
    if ((i+1) % cols == 0) printf("║\n");
    if (IsBinaryHeader(mem->values[i])) {
      i += PrintBinData(i, cell_width, cols, mem);
    } else if (IsBignumHeader(mem->values[i])) {
      u32 j;
      for (j = 0; j < (u32)Abs(RawInt(mem->values[i])); j++) {
        if ((i+j+1) % cols == 0) printf("║%04d║", i+j+1);
        else printf("│");
        printf("%*X", cell_width, mem->values[i+j+1]);
        if ((i+j+2) % cols == 0) printf("║\n");
      }
      i += Abs(RawInt(mem->values[i]));
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

typedef struct {
  u32 length;
  char *name;
} OpInfo;

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
  [OpTuple]   = "tuple",
  [OpSet]     = "set",
  [OpGet]     = "get",
  [OpCat]     = "cat",
  [OpExtend]  = "extend",
  [OpDefine]  = "define",
  [OpLookup]  = "lookup",
  [OpExport]  = "export",
  [OpJump]    = "jump",
  [OpBranch]  = "branch",
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
  printf("Env:\n");
  while (env != Nil) {
    Val frame = Head(env, mem);
    u32 i;
    printf("- ");
    for (i = 0; i < TupleLength(frame, mem); i++) {
      Val item = TupleGet(frame, i, mem);
      PrintVal(item, mem, symbols);
      printf(" ");
    }

    printf("\n");
    env = Tail(env, mem);
  }
}

void Disassemble(Chunk *chunk)
{
  u32 longest_sym, width, col1, col2;
  u32 i, line;
  char *sym;

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

void GenerateSymbols(void)
{
  printf("#define LinkReturn        0x%08X /* return */\n", SymbolFor("return"));
  printf("#define LinkNext          0x%08X /* next */\n", SymbolFor("next"));
  printf("#define SymEOF            0x%08X /* *eof* */\n", SymbolFor("*eof*"));
  printf("#define SymID             0x%08X /* *id* */\n", SymbolFor("*id*"));
  printf("#define SymBangEqual      0x%08X /* != */\n", SymbolFor("!="));
  printf("#define SymString         0x%08X /* \" */\n", SymbolFor("\""));
  printf("#define SymNewline        0x%08X /* \\n */\n", SymbolFor("\n"));
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
  printf("#define FuncType          0x%08X /* function */\n", SymbolFor("function"));
  printf("  {/* typeof */           0x%08X, &VMType},\n", SymbolFor("typeof"));
  printf("  {/* head */             0x%08X, &VMHead},\n", SymbolFor("head"));
  printf("  {/* tail */             0x%08X, &VMTail},\n", SymbolFor("tail"));
  printf("  {/* print */            0x%08X, &VMPrint},\n", SymbolFor("print"));
  printf("  {/* inspect */          0x%08X, &VMInspect},\n", SymbolFor("inspect"));
  printf("  {/* open */             0x%08X, &VMOpen},\n", SymbolFor("open"));
  printf("  {/* read */             0x%08X, &VMRead},\n", SymbolFor("read"));
  printf("  {/* write */            0x%08X, &VMWrite},\n", SymbolFor("write"));
  printf("  {/* ticks */            0x%08X, &VMTicks},\n", SymbolFor("ticks"));
  printf("  {/* seed */             0x%08X, &VMSeed},\n", SymbolFor("seed"));
  printf("  {/* random */           0x%08X, &VMRandom},\n", SymbolFor("random"));
  printf("  {/* sqrt */             0x%08X, &VMSqrt},\n", SymbolFor("sqrt"));
  printf("  {/* new_canvas */       0x%08X, &VMCanvas},\n", SymbolFor("new_canvas"));
  printf("  {/* draw_line */        0x%08X, &VMLine},\n", SymbolFor("draw_line"));
  printf("  {/* draw_text */        0x%08X, &VMText},\n", SymbolFor("draw_text"));
}

#endif
