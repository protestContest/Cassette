#include "ops.h"
#include "mem.h"
#include "leb.h"
#include "univ/math.h"
#include <univ/symbol.h>
#include <univ/vec.h>

char *OpName(OpCode op)
{
  switch (op) {
  case opNoop:    return "noop";
  case opConst:   return "const";
  case opAdd:     return "add";
  case opSub:     return "sub";
  case opMul:     return "mul";
  case opDiv:     return "div";
  case opRem:     return "rem";
  case opAnd:     return "and";
  case opOr:      return "or";
  case opComp:    return "comp";
  case opLt:      return "lt";
  case opGt:      return "gt";
  case opEq:      return "eq";
  case opNeg:     return "neg";
  case opNot:     return "not";
  case opShift:   return "shift";
  case opPair:    return "pair";
  case opHead:    return "head";
  case opTail:    return "tail";
  case opTuple:   return "tuple";
  case opLen:     return "len";
  case opGet:     return "get";
  case opSet:     return "set";
  case opStr:     return "str";
  case opJoin:    return "join";
  case opSlice:   return "slice";
  case opJump:    return "jump";
  case opBranch:  return "branch";
  case opTrap:    return "trap";
  case opPos:     return "pos";
  case opGoto:    return "goto";
  case opHalt:    return "halt";
  case opDup:     return "dup";
  case opDrop:    return "drop";
  case opSwap:    return "swap";
  case opOver:    return "over";
  case opRot:     return "rot";
  case opGetEnv:  return "getEnv";
  case opSetEnv:  return "setEnv";
  case opSetMod:  return "setMod";
  case opGetMod:  return "getMod";
  }
}

u32 DisassembleInst(u8 *code, u32 *index)
{
  OpCode op = code[*index];
  u32 arg;
  u32 len = 0;
  char *arg_str;
  u32 num_width = NumDigits(VecCount(code), 10);

  len += fprintf(stderr, "%*d│ %s", num_width, *index, OpName(op));
  (*index)++;

  switch (op) {
  case opConst:
    arg = ReadLEB(*index, code);
    arg_str = MemValStr(arg);
    len += fprintf(stderr, " %s", arg_str);
    free(arg_str);
    (*index) += LEBSize(arg);
    return len;
  case opTuple:
  case opBranch:
  case opJump:
  case opPos:
  case opTrap:
    arg = ReadLEB(*index, code);
    len += fprintf(stderr, " %d", arg);
    (*index) += LEBSize(arg);
    return len;
  default:
    return len;
  }
}

void Disassemble(u8 *code)
{
  u32 end = VecCount(code);
  u32 index = 0;
  u32 num_width = NumDigits(VecCount(code), 10);
  u32 i;

  if (!code) {
    fprintf(stderr, "--Empty--\n");
    return;
  }

  for (i = 0; i < num_width; i++) fprintf(stderr, "─");
  fprintf(stderr, "┬─disassembly─────\n");
  while (index < end) {
    DisassembleInst(code, &index);
    printf("\n");
  }
}
