#include "module.h"
#include "vm.h"
#include <univ.h>

void PushByte(u8 byte, Module *mod)
{
  VecPush(mod->code, byte);
}

void PushInt(i32 num, Module *mod)
{
  i32 length = 0;
  i32 test = num;
  i32 i;
  while (test > 0x7F) {
    test >>= 7;
    length++;
  }

  for (i = 0; i < length; i++) {
    u8 byte = ((num >> 7*i) | 0x80) & 0xFF;
    PushByte(byte, mod);
  }
  PushByte(num & 0x7F, mod);
}

i32 ReadInt(i32 *index, Module *mod)
{
  i8 byte = mod->code[(*index)++];
  i32 num = byte & 0x7F;
  while (byte & 0x80) {
    byte = mod->code[(*index)++];
    num = (num << 7) | (byte & 0x7F);
  }
  return num;
}

void PushConst(i32 c, Module *mod)
{
  u8 index = VecCount(mod->constants);
  VecPush(mod->constants, c);
  PushByte(opConst, mod);
  PushInt(index, mod);
}

void PushData(char *data, i32 length, Module *mod)
{
  char *dst = mod->data + VecCount(mod->data);
  GrowVec(mod->data, length);
  Copy(data, dst, length);
  VecPush(data, 0);
}

void InitProgramSymbols(Module *mod)
{
  char *data = mod->data;
  while (data < mod->data + VecCount(mod->data)) {
    Symbol(data);
    data += strlen(data) + 1;
  }
}

void Write(char *str, char *buf)
{
  i32 len = strlen(str);
  GrowVec(buf, len);
  Copy(str, buf + VecCount(buf) - len, len);
}

void WriteInt(i32 n, char *buf)
{
  i32 len = NumDigits(n, 10);
  GrowVec(buf, len);
  snprintf(buf + VecCount(buf) - len, VecCount(buf), "%d", n);
}

char *Disassemble(Module *mod)
{
  char *text = 0;
  i32 i = 0;
  i32 n;

  while (i < (i32)VecCount(mod->code)) {
    switch (mod->code[i]) {
    case opNoop:
      Write("noop\n", text);
      i++;
      break;
    case opConst:
      Write("const ", text);
      i++;
      n = ReadInt(&i, mod);
      WriteInt(n, text);
      Write("\n", text);
      break;
    case opAdd:
      Write("add\n", text);
      i++;
      break;
    case opSub:
      Write("sub\n", text);
      i++;
      break;
    case opMul:
      Write("mul\n", text);
      i++;
      break;
    case opDiv:
      Write("div\n", text);
      i++;
      break;
    case opRem:
      Write("rem\n", text);
      i++;
      break;
    case opAnd:
      Write("and\n", text);
      i++;
      break;
    case opOr:
      Write("or\n", text);
      i++;
      break;
    case opComp:
      Write("comp\n", text);
      i++;
      break;
    case opLt:
      Write("lt\n", text);
      i++;
      break;
    case opGt:
      Write("gt\n", text);
      i++;
      break;
    case opEq:
      Write("eq\n", text);
      i++;
      break;
    case opNeg:
      Write("neg\n", text);
      i++;
      break;
    case opShift:
      Write("shift\n", text);
      i++;
      break;
    case opNil:
      Write("nil\n", text);
      i++;
      break;
    case opPair:
      Write("pair\n", text);
      i++;
      break;
    case opHead:
      Write("head\n", text);
      i++;
      break;
    case opTail:
      Write("tail\n", text);
      i++;
      break;
    case opTuple:
      Write("tuple", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opLen:
      Write("len\n", text);
      i++;
      break;
    case opGet:
      Write("get\n", text);
      i++;
      break;
    case opSet:
      Write("set\n", text);
      i++;
      break;
    case opStr:
      Write("str\n", text);
      i++;
      break;
    case opBin:
      Write("bin\n", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opJmp:
      Write("jmp", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opBr:
      Write("br", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opTrap:
      Write("trap", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opPos:
      Write("pos\n", text);
      i++;
      break;
    case opGoto:
      Write("goto\n", text);
      i++;
      break;
    case opHalt:
      Write("halt\n", text);
      i++;
      break;
    case opDup:
      Write("dup\n", text);
      i++;
      break;
    case opDrop:
      Write("drop\n", text);
      i++;
      break;
    case opSwap:
      Write("swap\n", text);
      i++;
      break;
    case opOver:
      Write("over\n", text);
      i++;
      break;
    case opRot:
      Write("rot\n", text);
      i++;
      break;
    case opGetEnv:
      Write("genv\n", text);
      i++;
      break;
    case opSetEnv:
      Write("senv\n", text);
      i++;
      break;
    case opLookup:
      Write("lookup", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    case opDefine:
      Write("define", text);
      i++;
      WriteInt(ReadInt(&i, mod), text);
      Write("\n", text);
      break;
    }
  }
  return text;
}

void InitModuleSymbols(Module *m)
{

}
