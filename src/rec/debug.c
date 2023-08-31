#include "debug.h"
#include "heap.h"

void PrintSeq(Seq seq, Heap *mem)
{
  Val stmts = seq.stmts;

  while (!IsNil(stmts)) {
    Val stmt = Head(stmts, mem);

    // print source refs in first column
    if (IsTagged(stmt, "source-ref", mem)) {
      PrintIntN(RawInt(Tail(stmt, mem)), 3);
      Print(" ");
      while (IsTagged(stmt, "source-ref", mem)) {
        stmts = Tail(stmts, mem);
        stmt = Head(stmts, mem);
      }
    } else if (IsTagged(stmt, "source-file", mem)) {
      Print(SymbolName(Tail(stmt, mem), mem));
      Print(":\n");
      stmts = Tail(stmts, mem);
      continue;
    } else {
      Print("    ");
    }

    // labels in second column
    if (IsTagged(stmt, "label", mem)) {
      u32 label_num = RawInt(Tail(stmt, mem));
      Print("L");
      PrintInt(label_num);
      Print(":");
      stmts = Tail(stmts, mem);
    } else {
      OpCode op = RawInt(stmt);
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
        } else if (IsTagged(arg, "module-ref", mem)) {
          Inspect(Tail(arg, mem), mem);
        } else {
          Inspect(Head(stmts, mem), mem);
        }
        stmts = Tail(stmts, mem);
      }
    }
    Print("\n");
  }
}

void DisassemblePart(Chunk *chunk, u32 start, u32 count)
{
  if (start > VecCount(chunk->data)) start = VecCount(chunk->data);
  if (start + count > VecCount(chunk->data)) count = VecCount(chunk->data) - start;

  Print("────┬────────────\n");
  for (u32 i = start; i < start + count; i += OpLength(chunk->data[i])) {
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

void Disassemble(Chunk *chunk)
{
  DisassemblePart(chunk, 0, VecCount(chunk->data));
}

void PrintAST(Val ast, u32 indent, Heap *mem)
{
  Val expr = Tail(ast, mem);

  if (IsNil(expr)) {
  } else if (IsNum(expr)) {
    Inspect(expr, mem);
  } else if (IsTagged(expr, ":", mem)) {
    Inspect(Tail(Tail(expr, mem), mem), mem);
  } else if (IsTagged(expr, "\"", mem)) {
    Print("\"");
    Print(SymbolName(Tail(expr, mem), mem));
    Print("\"");
  } else if (Eq(expr, SymbolFor("nil"))) {
    Print("nil");
  } else if (Eq(expr, SymbolFor("true")))     {
    Print("true");
  } else if (Eq(expr, SymbolFor("false"))) {
    Print("false");
  } else if (IsSym(expr)) {
    Print(SymbolName(expr, mem));
  } else if (IsTagged(expr, "do", mem))      {
    Print("do");

    expr = Tail(expr, mem);
    while (!IsNil(expr)) {
      Print("\n");
      for (u32 i = 0; i < indent + 1; i++) Print("  ");
      PrintAST(Head(expr, mem), indent + 1, mem);
      expr = Tail(expr, mem);
    }
  } else if (IsTagged(expr, "module", mem)) {
    Print("module ");

    Val name = Tail(ListAt(expr, 1, mem), mem);
    Print(SymbolName(name, mem));
    expr = ListAt(expr, 2, mem);
    while (!IsNil(expr)) {
      Print("\n");
      for (u32 i = 0; i < indent + 1; i++) Print("  ");
      PrintAST(Head(expr, mem), indent + 1, mem);
      Print("\n");
      expr = Tail(expr, mem);
    }
  } else if (IsTagged(expr, "import", mem)) {
    Print("import ");
    expr = ListAt(expr, 1, mem);
    Print(SymbolName(Tail(ListAt(expr, 0, mem), mem), mem));
    Print(" as ");
    Print(SymbolName(Tail(ListAt(expr, 1, mem), mem), mem));
  } else if (IsTagged(expr, "let", mem)) {
    Print("let\n");
    expr = Tail(expr, mem);
    while (!IsNil(expr)) {
      Val assign = Head(expr, mem);
      Val var = Tail(Head(assign, mem), mem);
      for (u32 i = 0; i < indent + 1; i++) Print("  ");
      Print("[");
      Print(SymbolName(var, mem));
      Print(" = ");
      PrintAST(Head(Tail(assign, mem), mem), indent + 1, mem);
      Print("]");
      Print("\n");
      expr = Tail(expr, mem);
    }
  } else if (IsTagged(expr, "->", mem)) {
    Print("[");
    Print("-> [");
    Val args = Tail(ListAt(expr, 1, mem), mem);
    while (!IsNil(args)) {
      Val arg = Tail(Head(args, mem), mem);
      Print(SymbolName(arg, mem));
      if (!IsNil(Tail(args, mem))) Print(" ");
      args = Tail(args, mem);
    }
    Print("] ");
    PrintAST(ListAt(expr, 2, mem), indent + 1, mem);
    Print("]");
  } else if (IsTagged(expr, ".", mem) ||
   IsTagged(expr, "and", mem) ||
   IsTagged(expr, "or", mem) ||
   IsTagged(expr, "not", mem) ||
   IsTagged(expr, "#", mem)         ||
   IsTagged(expr, "*", mem)         ||
   IsTagged(expr, "/", mem)         ||
   IsTagged(expr, "%", mem)         ||
   IsTagged(expr, "+", mem)         ||
   IsTagged(expr, "-", mem) ||
   IsTagged(expr, "|", mem)         ||
   IsTagged(expr, "in", mem)        ||
   IsTagged(expr, ">", mem)         ||
   IsTagged(expr, "<", mem)         ||
   IsTagged(expr, "==", mem)) {
    Print("[");
    Print(SymbolName(Head(expr, mem), mem));
    expr = Tail(expr, mem);
    while (!IsNil(expr)) {
      Print(" ");
      PrintAST(Head(expr, mem), indent + 1, mem);
      expr = Tail(expr, mem);
    }
    Print("]");
  } else if (IsTagged(expr, "if", mem)) {
    Val pred = ListAt(expr, 1, mem);
    Val cons = ListAt(expr, 2, mem);
    Val alt = ListAt(expr, 3, mem);
    Print("if ");
    PrintAST(pred, indent + 1, mem);
    Print("\n");
    for (u32 i = 0; i < indent + 1; i++) Print("  ");
    PrintAST(cons, indent + 1, mem);
    Print("\n");
    for (u32 i = 0; i < indent + 1; i++) Print("  ");
    PrintAST(alt, indent + 1, mem);
  } else if (IsPair(expr)) {
    Print("[");
    while (!IsNil(expr)) {
      PrintAST(Head(expr, mem), indent + 1, mem);
      if (!IsNil(Tail(expr, mem))) {
        Print(" ");
        // for (u32 i = 0; i < indent + 1; i++) Print("  ");
      }
      expr = Tail(expr, mem);
    }
    Print("]");
    // Print("\n");
    // for (u32 i = 0; i < indent; i++) Print("  ");
  } else {
    Inspect(expr, mem);
  }
  // Print("\n");

  // Inspect(ast, mem);


  // if (IsTagged(expr, "[", mem))         return CompileList(Tail(expr, mem), linkage, c);
  // if (IsTagged(expr, "{", mem))         return CompileTuple(Tail(expr, mem), linkage, c);
  // if (IsTagged(expr, "{:", mem))        return CompileMap(Tail(expr, mem), linkage, c);

  // if (IsPair(expr))                     return CompileApplication(expr, linkage, c);

}
