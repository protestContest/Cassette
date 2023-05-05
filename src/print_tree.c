#include "print_tree.h"

static u32 Pad(u32 printed, u32 size, char *str)
{
  if (printed < size) {
    for (u32 i = 0; i < size - printed; i++) {
      Print(str);
    }
    return size - printed;
  }
  return 0;
}

static u32 Indent(u32 size, char *str)
{
  return Pad(0, size, str);
}

static u32 NodeWidth(Val node, Mem *mem)
{
  if (IsList(mem, node)) {
    Val op = Head(mem, node);
    return ValStrLen(mem, op);
  } else {
    return ValStrLen(mem, node);
  }
}

u32 NodesWidth(Val nodes, Mem *mem)
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

u32 TreeWidth(Val tree, Mem *mem)
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
  if (IsList(mem, node)) {
    len += PrintValStr(mem, Head(mem, node));
  } else {
    len += PrintValStr(mem, node);
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
      u32 node_width = NodeWidth(node, mem);
      if (IsList(mem, node)) {
        u32 width = NodesWidth(Tail(mem, node), mem);
        if (width < node_width) {
          // start += (NodeWidth(node, mem)-1)/2;
        }
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
