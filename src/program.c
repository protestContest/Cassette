#include "program.h"
#include "mem.h"
#include "ops.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"

Program *NewProgram(void)
{
  Program *program = malloc(sizeof(Program));
  program->code = 0;
  InitSourceMap(&program->srcmap);
  program->strings = 0;
  program->trace = false;
  return program;
}

void FreeProgram(Program *program)
{
  if (program->code) FreeVec(program->code);
  DestroySourceMap(&program->srcmap);
  if (program->strings) FreeVec(program->strings);
  free(program);
}

void AddStrings(ASTNode *node, Program *program, HashMap *strings)
{
  u32 i;
  if (node->type == strNode || node->type == symNode) {
    u32 len, sym;
    char *name;
    sym = RawVal(NodeValue(node));
    if (HashMapContains(strings, sym)) return;
    HashMapSet(strings, sym, 1);
    name = SymbolName(sym);
    len = strlen(name);
    if (!program->strings) {
      program->strings = NewVec(u8, len);
    }
    GrowVec(program->strings, len);
    Copy(name, VecEnd(program->strings) - len, len);
    VecPush(program->strings, 0);
    return;
  }
  if (IsTerminal(node)) return;
  for (i = 0; i < NodeCount(node); i++) {
    AddStrings(NodeChild(node, i), program, strings);
  }
}

void WriteBE(u32 num, u8 *dst)
{
  dst[0] = num >> 24;
  dst[1] = (num >> 16) & 0xFF;
  dst[2] = (num >> 8) & 0xFF;
  dst[3] = num & 0xFF;
}

u32 SerializeProgram(Program *program, u8 **dst)
{
  char form[4] = "FORM";
  char tape[4] = "TAPE";
  char code[4] = "CODE";
  char strs[4] = "STRS";

  u32 trailing_bytes, i;
  u32 code_size = Align(VecCount(program->code), sizeof(u32));
  u32 strs_size = Align(VecCount(program->strings), sizeof(u32));
  u32 form_size = sizeof(tape)
                + sizeof(code) + sizeof(code_size) + code_size
                + sizeof(strs) + sizeof(strs_size) + strs_size;
  u32 size = sizeof(form) + sizeof(form_size) + form_size;
  u8 *cur;
  *dst = realloc(*dst, size);
  cur = *dst;

  Copy(form, cur, sizeof(form));
  cur += sizeof(form);
  WriteBE(form_size, cur);
  cur += sizeof(form_size);
  Copy(tape, cur, sizeof(tape));
  cur += sizeof(tape);
  Copy(code, cur, sizeof(code));
  cur += sizeof(code);
  WriteBE(code_size, cur);
  cur += sizeof(code_size);
  Copy(program->code, cur, VecCount(program->code));
  cur += VecCount(program->code);
  trailing_bytes = code_size - VecCount(program->code);
  for (i = 0; i < trailing_bytes; i++) *cur++ = opNoop;
  Copy(strs, cur, sizeof(strs));
  cur += sizeof(strs);
  WriteBE(strs_size, cur);
  cur += sizeof(strs_size);
  Copy(program->strings, cur, VecCount(program->strings));
  cur += VecCount(program->strings);
  trailing_bytes = strs_size - VecCount(program->strings);
  for (i = 0; i < trailing_bytes; i++) *cur++ = 0;
  return size;
}

void DisassembleProgram(Program *program)
{
  u32 end = VecCount(program->code);
  u32 index = 0;
  u32 last_pos = 0;
  char *last_file = 0;

  while (index < end) {
    char *file = GetSourceFile(index, &program->srcmap);
    u32 pos = GetSourcePos(index, &program->srcmap);
    if (file != last_file) {
      fprintf(stderr, "%s\n", file ? file : "(system)");
      last_file = file;
      fprintf(stderr, "%4d:", pos);
      last_pos = pos;
    } else {
      if (pos != last_pos || index == 0) {
        fprintf(stderr, "%4d:", pos);
        last_pos = pos;
      } else {
        fprintf(stderr, "     ");
      }
    }

    DisassembleInst(program->code, &index);
    fprintf(stderr, "\n");
  }
}
