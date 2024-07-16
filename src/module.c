#include "module.h"
#include "vm.h"
#include <univ/vec.h>
#include <univ/symbol.h>
#include <univ/str.h>
#include <univ/math.h>

void InitModule(Module *mod)
{
  mod->constants = 0;
  mod->data = 0;
  mod->code = 0;
  mod->srcMap = 0;
}

void DestroyModule(Module *mod)
{
  FreeVec(mod->constants);
  FreeVec(mod->data);
  FreeVec(mod->code);
  FreeVec(mod->srcMap);
}

void PushPos(u32 pos, Module *mod)
{
  VecPush(mod->srcMap, pos);
}

void PushByte(u8 byte, u32 pos, Module *mod)
{
  VecPush(mod->code, byte);
  PushPos(pos, mod);
}

i32 IntSize(i32 num)
{
  i32 size = 0;
  while (num > 0x3F || num < -64) {
    size++;
    num >>= 7;
  }
  return size + 1;
}

void PushInt(i32 num, u32 pos, Module *mod)
{
  while (num > 0x3F || num < -64) {
    u8 byte = ((num & 0x7F) | 0x80) & 0xFF;
    PushByte(byte, pos, mod);
    num >>= 7;
  }
  PushByte(num & 0x7F, pos, mod);
}

i32 ReadInt(u32 *index, Module *mod)
{
  i8 byte = mod->code[(*index)++];
  i32 num = byte & 0x7F;
  u32 shift = 7;
  while (byte & 0x80) {
    byte = mod->code[(*index)++];
    num |= (byte & 0x7F) << shift;
    shift += 7;
  }
  if (shift < 8*sizeof(i32) && (num & 1<<(shift-1))) {
    num |= ~0 << shift;
  }
  return num;
}

void PushLabel(u32 index, u32 pos, Module *mod)
{
  i32 intSize = 1;
  i32 offset = index - (VecCount(mod->code) + intSize);
  while (IntSize(offset) > intSize) {
    intSize++;
    offset = index - (VecCount(mod->code) + intSize);
  }
  PushInt(offset, pos, mod);
}


void PushConst(val c, u32 pos, Module *mod)
{
  u8 index = VecCount(mod->constants);
  VecPush(mod->constants, c);
  PushByte(opConst, pos, mod);
  PushInt(index, pos, mod);
}

void PushData(char *data, i32 length, Module *mod)
{
  i32 index = VecCount(mod->data);
  GrowVec(mod->data, length);
  Copy(data, mod->data + index, length);
  VecPush(mod->data, 0);
}

char *DisassembleOp(u32 *index, char *text, Module *mod)
{
  i32 n;
  i32 col0 = NumDigits(VecCount(mod->code), 10);
  i32 nDigits = NumDigits(*index, 10);
  i32 j;
  char *str;
  for (j = 0; j < col0-nDigits; j++) text = BufWrite(" ", text);
  text = BufWriteNum(*index, text);
  text = BufWrite("  ", text);

  if (*index < VecCount(mod->code)) {
    switch (mod->code[(*index)++]) {
    case opNoop:
      text = BufWrite("noop", text);
      break;
    case opConst:
      text = BufWrite("const ", text);
      n = ReadInt(index, mod);
      str = MemValStr(mod->constants[n]);
      text = BufWrite(str, text);
      free(str);
      break;
    case opAdd:
      text = BufWrite("add", text);
      break;
    case opSub:
      text = BufWrite("sub", text);
      break;
    case opMul:
      text = BufWrite("mul", text);
      break;
    case opDiv:
      text = BufWrite("div", text);
      break;
    case opRem:
      text = BufWrite("rem", text);
      break;
    case opAnd:
      text = BufWrite("and", text);
      break;
    case opOr:
      text = BufWrite("or", text);
      break;
    case opComp:
      text = BufWrite("comp", text);
      break;
    case opLt:
      text = BufWrite("lt", text);
      break;
    case opGt:
      text = BufWrite("gt", text);
      break;
    case opEq:
      text = BufWrite("eq", text);
      break;
    case opNeg:
      text = BufWrite("neg", text);
      break;
    case opNot:
      text = BufWrite("not", text);
      break;
    case opShift:
      text = BufWrite("shift", text);
      break;
    case opNil:
      text = BufWrite("nil", text);
      break;
    case opPair:
      text = BufWrite("pair", text);
      break;
    case opHead:
      text = BufWrite("head", text);
      break;
    case opTail:
      text = BufWrite("tail", text);
      break;
    case opTuple:
      text = BufWrite("tuple ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opLen:
      text = BufWrite("len", text);
      break;
    case opGet:
      text = BufWrite("get", text);
      break;
    case opSet:
      text = BufWrite("set", text);
      break;
    case opStr:
      text = BufWrite("str", text);
      break;
    case opBin:
      text = BufWrite("bin ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opJoin:
      text = BufWrite("join", text);
      break;
    case opSlice:
      text = BufWrite("slice", text);
      break;
    case opJmp:
      text = BufWrite("jmp ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opBr:
      text = BufWrite("br ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opTrap:
      text = BufWrite("trap ", text);
      n = ReadInt(index, mod);
      text = BufWrite(SymbolName(n), text);
      break;
    case opPos:
      text = BufWrite("pos ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opGoto:
      text = BufWrite("goto", text);
      break;
    case opHalt:
      text = BufWrite("halt", text);
      break;
    case opDup:
      text = BufWrite("dup", text);
      break;
    case opDrop:
      text = BufWrite("drop", text);
      break;
    case opSwap:
      text = BufWrite("swap", text);
      break;
    case opOver:
      text = BufWrite("over", text);
      break;
    case opRot:
      text = BufWrite("rot", text);
      break;
    case opGetEnv:
      text = BufWrite("getenv", text);
      break;
    case opSetEnv:
      text = BufWrite("setenv", text);
      break;
    case opLookup:
      text = BufWrite("lookup ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    case opDefine:
      text = BufWrite("define ", text);
      text = BufWriteNum(ReadInt(index, mod), text);
      break;
    }
  }
  VecPush(text, 0);
  return text;
}

char *Disassemble(Module *mod)
{
  u32 index = 0;
  char *text = 0;
  while (index < VecCount(mod->code)) {
    text = DisassembleOp(&index, text, mod);
    text = BufWrite("\n", text);
    VecPush(text, 0);
  }
  return text;
}

u32 GetSourcePos(u32 index, Module *mod)
{
  if (index >= VecCount(mod->srcMap)) return 0;
  return mod->srcMap[index];
}
