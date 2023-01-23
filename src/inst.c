#include "inst.h"
#include "vm.h"
#include "value.h"
#include "printer.h"

const char *OpStr(OpCode op)
{
  switch (op) {
  case OP_NOOP:     return "NOOP";
  case OP_BRANCH:   return "BRANCH";
  case OP_ASSIGN:   return "ASSIGN";
  case OP_SAVE:     return "SAVE";
  case OP_RESTORE:  return "RESTORE";
  case OP_LOOKUP:   return "LOOKUP";
  case OP_DEFINE:   return "DEFINE";
  }
}

u32 PutByte(InstSeq seq, u32 i, u8 byte)
{
  seq.code[i] = byte;
  return i+1;
}

u32 PutVal(InstSeq seq, u32 i, Val val)
{
  *(Val*)&seq.code[i] = val;
  return i + sizeof(Val);
}

#define ReadVal(code, i)  (*(Val*)&(code)[i])

u32 PutInst(InstSeq seq, u32 i, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

  i = PutByte(seq, i, op);

  switch (op) {
  case OP_NOOP:
  case OP_BRANCH:
    break;
  case OP_SAVE:
  case OP_RESTORE:
    i = PutByte(seq, i, va_arg(args, Reg));
    break;
  case OP_ASSIGN:
    i = PutVal(seq, i, va_arg(args, Val));
    i = PutByte(seq, i, va_arg(args, Reg));
    break;
  case OP_LOOKUP:
    i = PutVal(seq, i, va_arg(args, Val));
    break;
  case OP_DEFINE:
    i = PutVal(seq, i, va_arg(args, Val));
    break;
  }

  return i;
}

InstSeq MakeInstSeq(u32 num_ops, ...)
{
  va_list count_args, args;
  va_start(count_args, num_ops);
  va_copy(args, count_args);

  u32 needs = 0;
  u32 modifies = 0;

  u32 size = 0;
  for (u32 i = 0; i < num_ops; i++) {
    OpCode op = va_arg(count_args, OpCode);
    size += 1;

    switch (op) {
    case OP_NOOP:
      break;
    case OP_ASSIGN:
      va_arg(count_args, Val);
      modifies = WithReg(modifies, va_arg(count_args, Reg));
      size += 5;
      break;
    case OP_BRANCH:
      needs = WithReg(needs, REG_CONT);
      break;
    case OP_SAVE:
      needs = WithReg(needs, va_arg(count_args, Reg));
      size += 1;
      break;
    case OP_RESTORE:
      modifies = WithReg(modifies, va_arg(count_args, Reg));
      size += 1;
      break;
    case OP_LOOKUP:
      va_arg(count_args, Val);
      needs = WithReg(needs, REG_ENV);
      modifies = WithReg(modifies, REG_VAL);
      size += 4;
      break;
    case OP_DEFINE:
      va_arg(count_args, Val);
      needs = WithReg(WithReg(needs, REG_VAL), REG_ENV);
      size += 4;
      break;
    }
  }

  va_end(count_args);

  InstSeq seq = { needs, modifies, size, malloc(size) };
  u32 c = 0;

  for (u32 i = 0; i < num_ops; i++) {
    OpCode op = va_arg(args, OpCode);
    c = PutByte(seq, c, op);

    switch (op) {
    case OP_NOOP:
      break;
    case OP_ASSIGN:
      c = PutVal(seq, c, va_arg(args, Val));
      c = PutByte(seq, c, va_arg(args, Reg));
      break;
    case OP_BRANCH:
      break;
    case OP_SAVE:
    case OP_RESTORE:
      c = PutByte(seq, c, va_arg(args, Reg));
      break;
    case OP_LOOKUP:
      c = PutVal(seq, c, va_arg(args, Val));
      break;
    case OP_DEFINE:
      c = PutVal(seq, c, va_arg(args, Val));
      break;
    }
  }

  va_end(args);

  return seq;
}

InstSeq EmptySeq(void)
{
  InstSeq seq = { 0, 0, 0, NULL };
  return seq;
}

u32 CopyCode(u32 start, InstSeq from, InstSeq to)
{
  for (u32 i = 0; i < from.size; i++) {
    to.code[start + i] = from.code[i];
  }
  return start + to.size;
}

InstSeq CombineSeqs(u32 needs, u32 modifies, InstSeq s1, InstSeq s2)
{
  u32 size = s1.size + s2.size;
  InstSeq seq = { needs, modifies, size, malloc(size) };
  u32 i = CopyCode(0, s1, seq);
  CopyCode(i, s2, seq);

  free(s1.code);
  free(s2.code);

  return seq;
}

InstSeq AppendSeqs(InstSeq s1, InstSeq s2)
{
  u32 needs = s1.needs | (s2.needs & s1.modifies);
  u32 modifies = s1.modifies | s2.modifies;
  return CombineSeqs(needs, modifies, s1, s2);
}

InstSeq SaveRestore(InstSeq s1, Reg reg)
{
  u32 needs = WithReg(s1.needs, reg);
  u32 modifies = WithoutReg(s1.modifies, reg);
  u32 size = s1.size + 4;

  InstSeq seq = {needs, modifies, size, malloc(size)};
  u32 i = PutInst(seq, 0, OP_SAVE, reg);
  i = CopyCode(i, s1, seq);
  PutInst(seq, i, OP_RESTORE, reg);

  free(s1.code);

  return seq;
}

InstSeq Preserving(u32 regs, InstSeq s1, InstSeq s2)
{
  if (regs == 0) {
    return AppendSeqs(s1, s2);
  }

  Reg reg = 0;
  for (; reg < NUM_REGS; reg++) {
    if ((regs & (1 << reg))) break;
  }

  InstSeq seq;
  if (IsRegNeeded(s2, reg) && IsRegModified(s1, reg)) {
    seq = Preserving(WithoutReg(regs, reg), SaveRestore(s1, reg), s2);
  } else {
    seq = Preserving(WithoutReg(regs, reg), s1, s2);
  }
  Disassemble("Preserving", seq);
  return seq;
}

InstSeq TackOnSeq(InstSeq s1, InstSeq s2)
{
  return CombineSeqs(s1.needs, s1.modifies, s1, s2);
}

InstSeq ParallelSeqs(InstSeq s1, InstSeq s2)
{
  u32 needs = s1.needs | s2.needs;
  u32 modifies = s1.modifies | s2.modifies;
  return CombineSeqs(needs, modifies, s1, s2);
}

void Disassemble(char *title, InstSeq seq)
{
  printf("== %s ==\n", title);
  printf("  Needs: ");
  if (seq.needs == 0) {
    printf("none\n");
  } else {
    for (Reg reg = 0; reg < NUM_REGS; reg++) {
      if (IsRegNeeded(seq, reg)) {
        printf("%s ", RegStr(reg));
      }
    }
    printf("\n");
  }

  printf("  Modifies: ");
  if (seq.needs == 0) {
    printf("none\n");
  } else {
    for (Reg reg = 0; reg < NUM_REGS; reg++) {
      if (IsRegNeeded(seq, reg)) {
        printf("%s ", RegStr(reg));
      }
    }
    printf("\n");
  }

  if (seq.size == 0) {
    printf("  Code: none\n");
  } else {
    printf("  Code:\n");
    for (u32 i = 0; i < seq.size;) {
      OpCode op = seq.code[i++];
      printf("    %s", OpStr(op));

      switch (op) {
      case OP_NOOP:
        break;
      case OP_BRANCH:
        break;
      case OP_ASSIGN:
        printf(" %s, %s", ValStr(ReadVal(seq.code, i)), RegStr(seq.code[i+4]));
        i += 5;
        break;
      case OP_SAVE:
      case OP_RESTORE:
        printf(" %s", RegStr(seq.code[i++]));
        break;
      case OP_LOOKUP:
      case OP_DEFINE:
        printf(" %s", ValStr(ReadVal(seq.code, i)));
        i += 4;
        break;
      }

      printf("\n");
    }
  }
}
