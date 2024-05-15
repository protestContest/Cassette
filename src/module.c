#include "module.h"
#include "vm.h"
#include <univ.h>

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

void AppendCode(u8 *code, u32 *srcMap, Module *mod)
{
  i32 index = VecCount(mod->code);
  GrowVec(mod->code, VecCount(code));
  Copy(code, mod->code + index, VecCount(code));
  index = VecCount(mod->srcMap);
  GrowVec(mod->srcMap, VecCount(srcMap));
  Copy(srcMap, mod->srcMap + index, sizeof(u32)*VecCount(srcMap));
  FreeVec(code);
  FreeVec(srcMap);
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

void InitProgramSymbols(Module *mod)
{
  char *data = mod->data;
  while (data < mod->data + VecCount(mod->data)) {
    Symbol(data);
    data += strlen(data) + 1;
  }
}

char *Write(char *str, char *buf)
{
  i32 len = strlen(str);
  if (VecCount(buf) > 0 && buf[VecCount(buf)-1] == 0) VecPop(buf);
  GrowVec(buf, len);
  Copy(str, buf + VecCount(buf) - len, len);
  return buf;
}

char *WriteInt(i32 n, char *buf)
{
  i32 len = NumDigits(n, 10);
  if (VecCount(buf) > 0 && buf[VecCount(buf)-1] == 0) VecPop(buf);
  GrowVec(buf, len+1);
  VecPop(buf);
  snprintf(buf + VecCount(buf) - len, VecCount(buf)+1, "%d", n);
  return buf;
}

char *DisassembleOp(u32 *index, char *text, Module *mod)
{
  i32 n;
  i32 col0 = NumDigits(VecCount(mod->code), 10);
  i32 nDigits = NumDigits(*index, 10);
  i32 j;
  char *str;
  for (j = 0; j < col0-nDigits; j++) text = Write(" ", text);
  text = WriteInt(*index, text);
  text = Write("  ", text);

  switch (mod->code[(*index)++]) {
  case opNoop:
    text = Write("noop", text);
    break;
  case opConst:
    text = Write("const ", text);
    n = ReadInt(index, mod);
    str = ValStr(mod->constants[n]);
    text = Write(str, text);
    free(str);
    break;
  case opAdd:
    text = Write("add", text);
    break;
  case opSub:
    text = Write("sub", text);
    break;
  case opMul:
    text = Write("mul", text);
    break;
  case opDiv:
    text = Write("div", text);
    break;
  case opRem:
    text = Write("rem", text);
    break;
  case opAnd:
    text = Write("and", text);
    break;
  case opOr:
    text = Write("or", text);
    break;
  case opComp:
    text = Write("comp", text);
    break;
  case opLt:
    text = Write("lt", text);
    break;
  case opGt:
    text = Write("gt", text);
    break;
  case opEq:
    text = Write("eq", text);
    break;
  case opNeg:
    text = Write("neg", text);
    break;
  case opNot:
    text = Write("not", text);
    break;
  case opShift:
    text = Write("shift", text);
    break;
  case opNil:
    text = Write("nil", text);
    break;
  case opPair:
    text = Write("pair", text);
    break;
  case opHead:
    text = Write("head", text);
    break;
  case opTail:
    text = Write("tail", text);
    break;
  case opTuple:
    text = Write("tuple ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opLen:
    text = Write("len", text);
    break;
  case opGet:
    text = Write("get", text);
    break;
  case opSet:
    text = Write("set", text);
    break;
  case opStr:
    text = Write("str", text);
    break;
  case opBin:
    text = Write("bin ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opJoin:
    text = Write("join", text);
    break;
  case opTrunc:
    text = Write("trunc", text);
    break;
  case opSkip:
    text = Write("skip", text);
    break;
  case opJmp:
    text = Write("jmp ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opBr:
    text = Write("br ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opTrap:
    text = Write("trap ", text);
    n = ReadInt(index, mod);
    text = Write(SymbolName(n), text);
    break;
  case opPos:
    text = Write("pos ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opGoto:
    text = Write("goto", text);
    break;
  case opHalt:
    text = Write("halt", text);
    break;
  case opDup:
    text = Write("dup", text);
    break;
  case opDrop:
    text = Write("drop", text);
    break;
  case opSwap:
    text = Write("swap", text);
    break;
  case opOver:
    text = Write("over", text);
    break;
  case opRot:
    text = Write("rot", text);
    break;
  case opGetEnv:
    text = Write("getenv", text);
    break;
  case opSetEnv:
    text = Write("setenv", text);
    break;
  case opLookup:
    text = Write("lookup ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
  case opDefine:
    text = Write("define ", text);
    text = WriteInt(ReadInt(index, mod), text);
    break;
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
    text = Write("\n", text);
    VecPush(text, 0);
  }
  return text;
}

u32 GetSourcePos(u32 index, Module *mod)
{
  if (index >= VecCount(mod->srcMap)) return 0;
  return mod->srcMap[index];
}
