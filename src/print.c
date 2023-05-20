#include "print.h"

#define PRINT_LIMIT 8

u32 PrintRawVal(Val value, Mem *mem)
{
  if (IsNum(value)) {
    return PrintFloat(RawNum(value), 3);
  } else if (IsInt(value)) {
    return PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    return PrintN(SymbolName(mem, value), SymbolLength(mem, value)) + 1;
  } else if (IsNil(value)) {
    return Print("nil");
  } else if (IsPair(value)) {
    Print("p");
    return PrintInt(RawVal(value)) + 1;
  } else if (IsTuple(mem, value)) {
    Print("t");
    return PrintInt(RawVal(value)) + 1;
  } else if (IsBinary(mem, value)) {
    Print("b");
    return PrintInt(RawVal(value)) + 1;
  } else if (IsBinaryHeader(value)) {
    Print("$");
    return PrintInt(RawVal(value)) + 1;
  } else if (IsTupleHeader(value)) {
    Print("#");
    return PrintInt(RawVal(value)) + 1;
  } else {
    Print("?");
    return PrintInt(RawVal(value)) + 1;
  }
}

void PrintRawValN(Val value, u32 size, Mem *mem)
{
  if (IsNum(value)) {
    PrintFloatN(RawNum(value), size);
  } else if (IsInt(value)) {
    PrintIntN(RawInt(value), size, ' ');
  } else if (IsSym(value)) {
    if (Eq(value, SymbolFor("false"))) {
      PrintN("false", size);
    } else if (Eq(value, SymbolFor("true"))) {
      PrintN("true", size);
    } else {
      u32 len = Min(size-1, SymbolLength(mem, value));
      for (u32 i = 0; i < size - 1 - len; i++) Print(" ");
      Print(":");
      PrintN(SymbolName(mem, value), len);
    }
  } else if (IsNil(value)) {
    PrintN("nil", size);
  } else {
    u32 digits = NumDigits(RawVal(value));
    if (digits < size-1) {
      for (u32 i = 0; i < size-1-digits; i++) Print(" ");
    }

    if (IsPair(value)) {
      Print("p");
    } else if (IsTuple(mem, value)) {
      Print("t");
    } else if (IsBinary(mem, value)) {
      Print("b");
    } else if (IsBinaryHeader(value)) {
      Print("$");
    } else if (IsTupleHeader(value)) {
      Print("#");
    } else {
      Print("?");
    }
    PrintInt(RawVal(value));
  }
}


static u32 Indent(u32 size, char *str)
{
  return Pad(0, size, str);
}

static u32 PrintSymbolVal(Val symbol, Mem *mem)
{
  u32 printed = 0;
  if (Eq(symbol, SymbolFor("true"))) {
    printed += Print("true");
  } else if (Eq(symbol, SymbolFor("false"))) {
    printed += Print("false");
  } else {
    printed += Print(":");
    printed += PrintSymbol(mem, symbol);
  }
  return printed;
}

static u32 PrintTuple(Val tuple, Mem *mem)
{
  u32 printed = 0;
  printed += Print("#[");
  u32 limit = TupleLength(mem, tuple);
  if (PRINT_LIMIT && PRINT_LIMIT < limit) limit = PRINT_LIMIT;
  for (u32 i = 0; i < limit; i++) {
    printed += PrintVal(mem, TupleAt(mem, tuple, i));
    if (i != TupleLength(mem, tuple) - 1) {
      printed += Print(", ");
    }
  }
  u32 rest = limit - TupleLength(mem, tuple);
  if (rest > 0) {
    printed += Print("...");
    printed += PrintInt(rest);
  }
  printed += Print("]");
  return printed;
}

static u32 PrintBinary(Val binary, Mem *mem)
{
  u32 printed = 0;
  u32 limit = BinaryLength(mem, binary);
  if (PRINT_LIMIT && PRINT_LIMIT*4 < limit) limit = PRINT_LIMIT*4;

  u8 *data = BinaryData(mem, binary);
  printed += Print("\"");
  for (u32 i = 0; i < limit; i++) {
    if (data[i] > 0x1F && data[i] < 0x7F) {
      printed += PrintChar(data[i]);
    } else {
      printed += Print(".");
    }
    printed += Print("\"");
  }

  return printed;
}

static u32 PrintTail(Mem *mem, Val tail, u32 max)
{
  u32 printed = 0;

  if (PRINT_LIMIT && max == 0) {
    printed += Print("...");
    printed += PrintInt(ListLength(mem, tail));
    printed += Print("]");
    return printed;
  }

  printed += PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    printed += Print("]");
  } else if (!IsPair(Tail(mem, tail))) {
    printed += Print(" | ");
    printed += PrintVal(mem, Tail(mem, tail));
    printed += Print("]");
  } else {
    printed += Print(" ");
    printed += PrintTail(mem, Tail(mem, tail), max - 1);
  }
  return printed;
}

u32 PrintVal(Mem *mem, Val value)
{
  u32 printed = 0;
  if (IsNil(value)) {
    printed += Print("nil");
  } else if (IsNum(value)) {
    printed += PrintFloat(value.as_f, 3);
  } else if (IsInt(value)) {
    printed += PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    printed += PrintSymbolVal(value, mem);
  } else if (IsPair(value)) {
    printed += Print("[");
    printed += PrintTail(mem, value, PRINT_LIMIT);
  } else if (IsTuple(mem, value)) {
    PrintTuple(value, mem);
  } else if (IsBinary(mem, value)) {
    u32 length = Min(16, BinaryLength(mem, value));
    u8 *data = BinaryData(mem, value);
    printed += Print("\"");
    for (u32 i = 0; i < length; i++) {
      if (data[i] > 31) {
        char c[2] = {data[i], '\0'};
        printed += Print(c);
      } else {
        printed += Print(".");
      }
    }
    printed += Print("\"");
  } else {
    printed += Print("<v");
    printed += PrintIntN(RawVal(value), 4, ' ');
    printed += Print(">");
  }

  return printed;
}

static u32 PrintValLen(Mem *mem, Val value)
{
  if (IsNil(value)) {
    return 3;
  } else if (IsNum(value)) {
    return NumFloatDigits(value.as_f);
  } else if (IsInt(value)) {
    return NumDigits(RawInt(value));
  } else if (IsSym(value)) {
    return SymbolLength(mem, value);
  } else if (IsPair(value)) {
    return 0;
  } else if (IsTuple(mem, value)) {
    u32 printed = 2;
    u32 limit = TupleLength(mem, value);
    if (PRINT_LIMIT && PRINT_LIMIT < limit) limit = PRINT_LIMIT;
    for (u32 i = 0; i < limit; i++) {
      printed += PrintValLen(mem, TupleAt(mem, value, i));
      if (i != TupleLength(mem, value) - 1) {
        printed += 2;
      }
    }
    u32 rest = limit - TupleLength(mem, value);
    if (rest > 0) {
      printed += 3;
      printed += NumDigits(rest);
    }
    printed += 1;
    return printed;
  } else if (IsBinary(mem, value)) {
    u32 length = Min(16, BinaryLength(mem, value));
    return length + 2;
  } else {
    return 7;
  }
}

static u32 NodeWidth(Val node, Mem *mem)
{
  if (IsList(mem, node)) {
    Val op = Head(mem, node);
    return PrintValLen(mem, op);
  } else {
    return PrintValLen(mem, node);
  }
}

static u32 NodesWidth(Val nodes, Mem *mem)
{
  Assert(IsList(mem, nodes));
  u32 width = 0;
  while (!IsNil(nodes)) {
    width += NodeWidth(Head(mem, nodes), mem);
    if (!IsNil(Tail(mem, nodes))) width += 1;
    nodes = Tail(mem, nodes);
  }
  return width;
}

static u32 TreeWidth(Val tree, Mem *mem)
{
  if (!IsList(mem, tree)) return NodeWidth(tree, mem);
  Val op = Head(mem, tree);
  Val args = Tail(mem, tree);

  u32 width = 0;
  while (!IsNil(args)) {
    width += TreeWidth(Head(mem, args), mem);
    if (!IsNil(Tail(mem, args))) width += 1;
    args = Tail(mem, args);
  }
  return Max(width, NodeWidth(op, mem));
}

static u32 NodeOffset(Val node, Mem *mem);
static u32 ParentOffset(Val nodes, Mem *mem)
{
  if (IsNil(nodes)) return 0;

  u32 width = 0;
  while (!IsNil(nodes)) {
    if (IsNil(Tail(mem, nodes))) {
      width += NodeOffset(Head(mem, nodes), mem);
    } else {
      width += TreeWidth(Head(mem, nodes), mem);
    }
    nodes = Tail(mem, nodes);
  }
  return width/2;
}

static u32 NodeOffset(Val node, Mem *mem)
{
  if (!IsList(mem, node)) return 0;
  Val args = Tail(mem, node);

  u32 node_width = NodeWidth(node, mem);
  u32 width = ParentOffset(args, mem);

  if (node_width/2 < width) {
    return width - (node_width-1)/2;
  } else {
    return 0;
  }
}

static u32 PrintTreeNode(Val node, Mem *mem)
{
  u32 width = TreeWidth(node, mem);
  u32 len = 0;
  len += Indent(NodeOffset(node, mem), " ");
  if (IsList(mem, node)) node = Head(mem, node);

  if (IsSym(node)) {
    len += PrintSymbol(mem, node);
  } else {
    len += PrintVal(mem, node);
  }

  if (width > len) Indent(width - len, " ");
  Print(" ");
  return Max(width, len) + 1;
}

static Val PrintTreeLevel(Val trees, Mem *mem)
{
  u32 pos = 0;
  Val next = nil;
  while (!IsNil(trees)) {
    Val tree = Head(mem, trees);
    u32 start = RawInt(Head(mem, tree));
    if (start > pos) pos += Indent(start - pos, " ");

    Val nodes = Tail(mem, tree);
    while (!IsNil(nodes)) {
      Val node = Head(mem, nodes);
      u32 start = pos;
      u32 len = PrintTreeNode(node, mem);
      if (IsList(mem, node)) {
        Val children = MakePair(mem, IntVal(start), Tail(mem, node));
        next = ListAppend(mem, next, children);
      }
      pos += len;
      nodes = Tail(mem, nodes);
    }

    trees = Tail(mem, trees);
  }
  Print("\n");
  return next;
}

static void PrintTreeLines(Val trees, Mem *mem)
{
  u32 pos = 0;
  while (!IsNil(trees)) {
    Val tree = Head(mem, trees);
    u32 start = RawInt(Head(mem, tree));

    if (start > pos) pos += Indent(start - pos, " ");

    Val nodes = Tail(mem, tree);
    u32 parent_offset = ParentOffset(nodes, mem);

    bool head = true;
    while (!IsNil(nodes)) {
      Val node = Head(mem, nodes);
      bool tail = IsNil(Tail(mem, nodes));

      u32 width = TreeWidth(node, mem);
      Assert(NodeWidth(node, mem) > 0);
      u32 offset = NodeOffset(node, mem) + (NodeWidth(node, mem)-1)/2;
      if (head) {
        if (offset > parent_offset) {
          Print("└");
          Indent(offset-1, "─");
        } else {
          Indent(offset, " ");
        }
        pos += offset;
        if (tail) {
          if (pos - start == parent_offset) {
            Print("│");
          } else {
            Print("┐");
          }
        } else if (pos - start == parent_offset) {
          Print("├");
        } else {
          Print("┌");
        }
        pos++;
      } else {
        for (u32 i = 0; i < offset; i++) {
          if (pos - start == parent_offset) {
            Print("┴");
          } else {
            Print("─");
          }
          pos++;
        }
        if (tail) {
          Print("┐");
        } else {
          if (pos - start == parent_offset) {
            Print("┼");
          } else {
            Print("┬");
          }
        }
        pos++;
      }

      if (!tail) {
        for (u32 i = 0; i < width - offset; i++) {
          if (pos - start == parent_offset) {
            Print("┴");
          } else {
            Print("─");
          }
          pos++;
        }
      } else {
        pos += Indent(width - offset, " ");
      }

      head = false;
      nodes = Tail(mem, nodes);
    }

    trees = Tail(mem, trees);
  }
  Print("\n");
}

void PrintTree(Val tree, Mem *mem)
{
  Val nodes = MakePair(mem, IntVal(0), MakePair(mem, tree, nil));
  Val queue = MakePair(mem, nodes, nil);
  queue = PrintTreeLevel(queue, mem);
  while (!IsNil(queue)) {
    PrintTreeLines(queue, mem);
    queue = PrintTreeLevel(queue, mem);
  }
}
