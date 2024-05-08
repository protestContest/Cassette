#include "program.h"
#include "vm.h"
#include <univ.h>

void PushByte(u8 byte, Program *p)
{
  VecPush(p->code, byte);
}

void PushInt(i32 num, Program *p)
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
    PushByte(byte, p);
  }
  PushByte(num & 0x7F, p);
}

i32 ReadInt(i32 *index, Program *p)
{
  i8 byte = p->code[(*index)++];
  i32 num = byte & 0x7F;
  while (byte & 0x80) {
    byte = p->code[(*index)++];
    num = (num << 7) | (byte & 0x7F);
  }
  return num;
}

void PushConst(i32 c, Program *p)
{
  u8 index = VecCount(p->constants);
  VecPush(p->constants, c);
  PushByte(opConst, p);
  PushInt(index, p);
}

void PushData(char *data, i32 length, Program *p)
{
  char *dst = p->data + VecCount(p->data);
  GrowVec(p->data, length);
  Copy(data, dst, length);
  VecPush(data, 0);
}

void InitProgramSymbols(Program *p)
{
  char *data = p->data;
  while (data < p->data + VecCount(p->data)) {
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

char *Disassemble(Program *p)
{
  char *text = 0;
  i32 i = 0;
  i32 n;

  while (i < (i32)VecCount(p->code)) {
    switch (p->code[i]) {
    case opNoop:
      Write("noop\n", text);
      i++;
      break;
    case opConst:
      Write("const ", text);
      i++;
      n = ReadInt(&i, p);
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
      WriteInt(ReadInt(&i, p), text);
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
      WriteInt(ReadInt(&i, p), text);
      Write("\n", text);
      break;
    case opJmp:
      Write("jmp", text);
      i++;
      WriteInt(ReadInt(&i, p), text);
      Write("\n", text);
      break;
    case opBr:
      Write("br", text);
      i++;
      WriteInt(ReadInt(&i, p), text);
      Write("\n", text);
      break;
    case opTrap:
      Write("trap", text);
      i++;
      WriteInt(ReadInt(&i, p), text);
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
      WriteInt(ReadInt(&i, p), text);
      Write("\n", text);
      break;
    case opDefine:
      Write("define", text);
      i++;
      WriteInt(ReadInt(&i, p), text);
      Write("\n", text);
      break;
    }
  }
  return text;
}
