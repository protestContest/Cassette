#include "chunk.h"
#include "vm.h"
#include <univ/symbol.h>

val MakeChunk(Regs needs, Regs modifies, val code)
{
  val chunk = Tuple(4);
  TupleSet(chunk, 0, SymVal(Symbol("chunk")));
  TupleSet(chunk, 1, IntVal(needs));
  TupleSet(chunk, 2, IntVal(modifies));
  TupleSet(chunk, 3, code);
  return chunk;
}

val AppendChunk(val a, val b)
{
  Regs modifies = ChunkModifies(a) | ChunkModifies(b);
  Regs needs = ChunkNeeds(a) | (ChunkNeeds(b) & ~ChunkModifies(a));
  if (!ChunkCode(a)) return b;
  if (!ChunkCode(b)) return a;
  return MakeChunk(needs, modifies, Pair(ChunkCode(a), Pair(ChunkCode(b), 0)));
}

val AppendChunks(val chunks)
{
  if (!chunks) return EmptyChunk();
  return AppendChunk(Head(chunks), AppendChunks(Tail(chunks)));
}

/* ensures a doesn't mangle regs for use in b */
val Preserving(i32 regs, val a, val b)
{
  val code = ChunkCode(a);
  i32 modifies = ChunkModifies(a);
  i32 needs = ChunkNeeds(a);
  if (!regs) return AppendChunk(a, b);

  if ((ChunkNeeds(b) & regEnv) && (ChunkModifies(a) & regEnv)) {
    modifies &= ~regEnv;
    needs |= regEnv;
    code =
      Pair(Op("getEnv"),
      Pair(code,
      Pair(Op("swap"),
      Pair(Op("setEnv"), 0))));
  }

  if ((ChunkNeeds(b) & regCont) && (ChunkModifies(a) & regCont)) {
    modifies &= ~regCont;
    needs |= regCont;
    code =
      Pair(Op("getCont"),
      Pair(code,
      Pair(Op("swap"),
      Pair(Op("setCont"), 0))));
  }

  return AppendChunk(MakeChunk(needs, modifies, code), b);
}

val AppendCode(val a, val code)
{
  code = Pair(ChunkCode(a), Pair(code, 0));
  return MakeChunk(ChunkNeeds(a), ChunkModifies(a), code);
}

val ParallelChunks(val a, val b)
{
  i32 needs = ChunkNeeds(a) | ChunkNeeds(b);
  i32 modifies = ChunkModifies(a) | ChunkModifies(b);
  val code = Pair(ChunkCode(a), Pair(ChunkCode(b), 0));
  return MakeChunk(needs, modifies, code);
}

val TackOnChunk(val a, val b)
{
  return MakeChunk(ChunkNeeds(a), ChunkModifies(a),
    Pair(ChunkCode(a), Pair(ChunkCode(b), 0)));
}

val LabelChunk(val label, val chunk)
{
  val code = Pair(SymVal(Symbol("label")), Pair(label, ChunkCode(chunk)));
  return MakeChunk(ChunkNeeds(chunk), ChunkModifies(chunk), code);
}

val PrintStmt(val code)
{
  val op = Head(code);
  code = Tail(code);

  if (!IsSym(op)) {
    printf("%s\n", MemValStr(op));
  }

  assert(IsSym(op));

  if (op == Op("label")) {
    printf("lbl%d:", RawInt(Head(code)));
    code = Tail(code);
  } else {
    printf("  %s", SymbolName(RawVal(op)));
  }

  if (op == Op("const") || op == Op("trap")) {
    printf(" %s", MemValStr(Head(code)));
    code = Tail(code);
  }
  if (op == Op("lookup") || op == Op("tuple") || op == Op("define")) {
    printf(" %d", RawInt(Head(code)));
    code = Tail(code);
  }
  if (op == Op("branch") || op == Op("pos")) {
    printf(" lbl%d", RawInt(Head(code)));
    code = Tail(code);
  }

  printf("\n");
  return code;
}

void PrintChunkCode(val code)
{
  if (!code) return;

  if (IsPair(Head(code))) {
    PrintChunkCode(Head(code));
    PrintChunkCode(Tail(code));
  } else {
    PrintChunkCode(PrintStmt(code));
  }
}

void PrintChunk(val chunk)
{
  printf("Needs: %d, Modifies: %d\n", ChunkNeeds(chunk), ChunkModifies(chunk));
  PrintChunkCode(ChunkCode(chunk));
}
