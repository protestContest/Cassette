#include "chunk.h"
#include "vm.h"
#include "printer.h"
#include "mem.h"

typedef struct {
  OpCode op;
  char *name;
  ArgFormat format;
} OpInfo;

static OpInfo ops[] = {
  [OP_LABEL] =    { OP_LABEL,   "label ", ARGS_VAL      },
  [OP_MOVE] =     { OP_MOVE,    "move ",  ARGS_REG_REG  },
  [OP_ASSIGN] =   { OP_ASSIGN,  "assign", ARGS_VAL_REG  },
  [OP_TEST] =     { OP_TEST,    "test  ", ARGS_VAL_REG  },
  [OP_BRANCH] =   { OP_BRANCH,  "branch", ARGS_VAL      },
  [OP_GOTO] =     { OP_BRANCH,  "goto  ", ARGS_VAL      },
  [OP_SAVE] =     { OP_SAVE,    "push  ", ARGS_REG      },
  [OP_RESTORE] =  { OP_RESTORE, "pop   ", ARGS_REG      },
  [OP_ADD] =      { OP_ADD,     "add   ", ARGS_VAL_REG  },
};

const char *OpStr(OpCode op)
{
  return ops[op].name;
}

ArgFormat OpFormat(OpCode op)
{
  return ops[op].format;
}

u32 OpSize(OpCode op)
{
  switch (OpFormat(op)) {
  case ARGS_NONE:     return 1;
  case ARGS_REG:      return 2;
  case ARGS_VAL:      return 5;
  case ARGS_REG_REG:  return 3;
  case ARGS_VAL_REG:  return 6;
  }
}

void PutByte(Chunk *chunk, u8 byte)
{
  chunk->code[chunk->pos++] = byte;
}

void PutVal(Chunk *chunk, Val val)
{
  *(Val*)&chunk->code[chunk->pos] = val;
  chunk->pos += sizeof(Val);
}

u8 ReadByte(Chunk *chunk)
{
  return chunk->code[chunk->pos++];
}

Val ReadVal(Chunk *chunk)
{
  Val val = (*(Val*)&(chunk->code)[chunk->pos]);
  chunk->pos += sizeof(Val);
  return val;
}

void PutInst(Chunk *chunk, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

  PutByte(chunk, op);

  switch (OpFormat(op)) {
  case ARGS_NONE:
    break;
  case ARGS_REG:
    PutByte(chunk, va_arg(args, Reg));
    break;
  case ARGS_VAL:
    PutVal(chunk, va_arg(args, Val));
    break;
  case ARGS_REG_REG:
    PutByte(chunk, va_arg(args, Reg));
    PutByte(chunk, va_arg(args, Reg));
    break;
  case ARGS_VAL_REG:
    PutVal(chunk, va_arg(args, Val));
    PutByte(chunk, va_arg(args, Reg));
    break;
  }

  va_end(args);
}

Chunk *NewChunk(u32 needs, u32 modifies, u32 size)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  chunk->needs = needs;
  chunk->modifies = modifies;
  chunk->size = size;
  chunk->pos = 0;
  chunk->code = malloc(size);
  return chunk;
}

Chunk *MakeChunk(u32 num_ops, ...)
{
  va_list count_args, args;
  va_start(count_args, num_ops);
  va_copy(args, count_args);

  u32 needs = 0;
  u32 modifies = 0;

  u32 size = 0;
  for (u32 i = 0; i < num_ops; i++) {
    OpCode op = va_arg(count_args, OpCode);
    size += OpSize(op);

    switch (op) {
    case OP_LABEL:
      va_arg(count_args, Val);
      break;
    case OP_MOVE:
      needs = WithReg(needs, va_arg(count_args, Reg));
      modifies = WithReg(modifies, va_arg(count_args, Reg));
      break;
    case OP_ASSIGN:
      va_arg(count_args, Val);
      modifies = WithReg(modifies, va_arg(count_args, Reg));
      break;
    case OP_TEST:
      va_arg(count_args, Val);
      needs = WithReg(needs, va_arg(count_args, Reg));
      break;
    case OP_BRANCH:
      va_arg(count_args, Val);
      break;
    case OP_GOTO:
      va_arg(count_args, Val);
      break;
    case OP_SAVE:
      needs = WithReg(needs, va_arg(count_args, Reg));
      break;
    case OP_RESTORE:
      modifies = WithReg(modifies, va_arg(count_args, Reg));
      break;
    case OP_ADD:{
      va_arg(count_args, Val);
      Reg reg = va_arg(count_args, Reg);
      needs = WithReg(needs, reg);
      modifies = WithReg(modifies, reg);
      break;
    }
    }
  }
  va_end(count_args);

  Chunk *chunk = NewChunk(needs, modifies, size);

  for (u32 i = 0; i < num_ops; i++) {
    OpCode op = va_arg(args, OpCode);
    PutByte(chunk, op);

    switch (OpFormat(op)) {
    case ARGS_NONE:
      break;
    case ARGS_REG:
      PutByte(chunk, va_arg(args, Reg));
      break;
    case ARGS_VAL:
      PutVal(chunk, va_arg(args, Val));
      break;
    case ARGS_REG_REG:
      PutByte(chunk, va_arg(args, Reg));
      PutByte(chunk, va_arg(args, Reg));
      break;
    case ARGS_VAL_REG:
      PutVal(chunk, va_arg(args, Val));
      PutByte(chunk, va_arg(args, Reg));
      break;
    }
  }

  va_end(args);

  return chunk;
}

Chunk *EmptyChunk(void)
{
  return NewChunk(0, 0, 0);
}

Label *AddLabel(Label *labels, Val name, u32 pos)
{
  Label *label = malloc(sizeof(Label));
  label->name = name;
  label->pos = pos;
  label->next = labels;
  return label;
}

Val LabelPos(Label *labels, Val name)
{
  Label *label = labels;
  while (label != NULL) {
    if (Eq(label->name, name)) {
      return IntVal(label->pos);
    }
    label = label->next;
  }

  return nil;
}

Val LabelAt(Label *labels, u32 pos)
{
  Label *label = labels;
  while (label != NULL) {
    if (label->pos == pos) {
      return label->name;
    }
  }
  return nil;
}

void Assemble(Chunk *chunk)
{
  Label *labels = NULL;

  chunk->pos = 0;
  while (chunk->pos < chunk->size) {
    OpCode op = ReadByte(chunk);
    if (op == OP_LABEL) {
      Val label = ReadVal(chunk);
      if (IsSym(label)) {
        labels = AddLabel(labels, label, chunk->pos);
      }
    } else {
      chunk->pos += OpSize(op) - 1;
    }
  }

  chunk->pos = 0;
  while (chunk->pos < chunk->size) {
    OpCode op = ReadByte(chunk);
    if (op == OP_BRANCH || op == OP_GOTO) {
      Val label = ReadVal(chunk);
      if (IsSym(label)) {
        chunk->pos -= sizeof(Val);
        PutVal(chunk, LabelPos(labels, label));
      }
    } else {
      chunk->pos += OpSize(op) - 1;
    }
  }
}

u32 DisassembleInstruction(Chunk *chunk)
{
  OpCode op = ReadByte(chunk);
  u32 written = 0;

  switch (OpFormat(op)) {
  case ARGS_NONE:
    break;
  case ARGS_REG:
    written = printf("  %s %s", OpStr(op), RegStr(ReadByte(chunk)));
    break;
  case ARGS_VAL:
    written = printf("  %s %s", OpStr(op), ValStr(ReadVal(chunk)));
    break;
  case ARGS_REG_REG:
    written = printf("  %s %s %s", OpStr(op), RegStr(ReadByte(chunk)), RegStr(ReadByte(chunk)));
    break;
  case ARGS_VAL_REG:
    written = printf("  %s %s %s", OpStr(op), ValStr(ReadVal(chunk)), RegStr(ReadByte(chunk)));
    break;
  }

  return written;
}

void Disassemble(Chunk *chunk)
{
  chunk->pos = 0;
  while (chunk->pos < chunk->size) {
    DisassembleInstruction(chunk);
    printf("\n");
  }
}

void PrintChunk(char *title, Chunk *chunk)
{

  printf("━━╸%s╺━━\n", title);
  printf("Needs     0x%02X ", chunk->needs);
  if (chunk->needs != 0) {
    for (Reg reg = 0; reg < NUM_REGS; reg++) {
      if (IsRegNeeded(chunk, reg)) {
        printf("%s ", RegStr(reg));
      }
    }
  }
  printf("\n");

  printf("Modifies  0x%02X ", chunk->modifies);
  if (chunk->modifies != 0) {
    for (Reg reg = 0; reg < NUM_REGS; reg++) {
      if (IsRegModified(chunk, reg)) {
        printf("%s ", RegStr(reg));
      }
    }
  }
  printf("\n");

  Disassemble(chunk);
}

// u32 CopyCode(u32 start, Chunk from, Chunk to)
// {
//   for (u32 i = 0; i < from.size; i++) {
//     to.code[start + i] = from.code[i];
//   }
//   return start + to.size;
// }

// Chunk CombineSeqs(u32 needs, u32 modifies, Chunk s1, Chunk s2)
// {
//   u32 size = s1.size + s2.size;
//   Chunk seq = { needs, modifies, size, malloc(size) };
//   u32 i = CopyCode(0, s1, seq);
//   CopyCode(i, s2, seq);

//   free(s1.code);
//   free(s2.code);

//   return seq;
// }

// Chunk AppendSeqs(Chunk s1, Chunk s2)
// {
//   u32 needs = s1.needs | (s2.needs & s1.modifies);
//   u32 modifies = s1.modifies | s2.modifies;
//   return CombineSeqs(needs, modifies, s1, s2);
// }

// Chunk SaveRestore(Chunk s1, Reg reg)
// {
//   u32 needs = WithReg(s1.needs, reg);
//   u32 modifies = WithoutReg(s1.modifies, reg);
//   u32 size = s1.size + 4;

//   Chunk seq = {needs, modifies, size, malloc(size)};
//   u32 i = PutInst(seq, 0, OP_SAVE, reg);
//   i = CopyCode(i, s1, seq);
//   PutInst(seq, i, OP_RESTORE, reg);

//   free(s1.code);

//   return seq;
// }

// Chunk Preserving(u32 regs, Chunk s1, Chunk s2)
// {
//   if (regs == 0) {
//     return AppendSeqs(s1, s2);
//   }

//   Reg reg = 0;
//   for (; reg < NUM_REGS; reg++) {
//     if ((regs & (1 << reg))) break;
//   }

//   Chunk seq;
//   if (IsRegNeeded(s2, reg) && IsRegModified(s1, reg)) {
//     seq = Preserving(WithoutReg(regs, reg), SaveRestore(s1, reg), s2);
//   } else {
//     seq = Preserving(WithoutReg(regs, reg), s1, s2);
//   }
//   return seq;
// }

// Chunk TackOnSeq(Chunk s1, Chunk s2)
// {
//   return CombineSeqs(s1.needs, s1.modifies, s1, s2);
// }

// Chunk ParallelSeqs(Chunk s1, Chunk s2)
// {
//   u32 needs = s1.needs | s2.needs;
//   u32 modifies = s1.modifies | s2.modifies;
//   return CombineSeqs(needs, modifies, s1, s2);
// }
