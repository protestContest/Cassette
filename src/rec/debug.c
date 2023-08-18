#include "debug.h"
#include "heap.h"

void PrintSeq(Seq seq, Heap *mem)
{
  Val stmts = seq.stmts;

  while (!IsNil(stmts)) {
    if (IsTagged(Head(stmts, mem), "label", mem)) {
      u32 label_num = RawInt(Tail(Head(stmts, mem), mem));
      Print("L");
      PrintInt(label_num);
      Print(":");
      stmts = Tail(stmts, mem);
    } else {
      OpCode op = RawInt(Head(stmts, mem));
      Print("  ");
      Print(OpName(op));
      stmts = Tail(stmts, mem);
      for (u32 i = 0; i < OpLength(op) - 1; i++) {
        Print(" ");
        Val arg = Head(stmts, mem);
        if (IsTagged(arg, "label-ref", mem)) {
          u32 label_num = RawInt(Tail(arg, mem));
          Print("L");
          PrintInt(label_num);
        } else {
          Inspect(Head(stmts, mem), mem);
        }
        stmts = Tail(stmts, mem);
      }
    }
    Print("\n");
  }
}

void Disassemble(Chunk *chunk)
{
  Print("────┬────────────\n");
  for (u32 i = 0; i < VecCount(chunk->data); i += OpLength(chunk->data[i])) {
    PrintIntN(i, 4);
    Print("│ ");
    Print(OpName(chunk->data[i]));
    Print(" ");

    for (u32 j = 0; j < OpLength(chunk->data[i]) - 1; j++) {
      DebugVal(ChunkConst(chunk, i+j+1), &chunk->constants);
      Print(" ");
    }
    Print("\n");
  }
  Print("────┴────────────\n");
}
