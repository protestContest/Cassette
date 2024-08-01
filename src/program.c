#include "program.h"
#include "univ/str.h"
#include <univ/vec.h>

Program *NewProgram(void)
{
  Program *program = malloc(sizeof(Program));
  program->code = 0;
  program->strings = 0;
  return program;
}

void FreeProgram(Program *program)
{
  if (program->code) FreeVec(program->code);
  if (program->strings) FreeVec(program->strings);
  free(program);
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
  char form[4] = {'F', 'O', 'R', 'M'};
  char csst[4] = {'C', 'S', 'S', 'T'};
  char code[4] = {'C', 'O', 'D', 'E'};
  char strs[4] = {'S', 'T', 'R', 'S'};
  u32 code_size = VecCount(program->code);
  u32 strs_size = VecCount(program->strings);
  u32 form_size = sizeof(csst)
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
  Copy(csst, cur, sizeof(csst));
  cur += sizeof(csst);
  Copy(code, cur, sizeof(code));
  cur += sizeof(code);
  WriteBE(code_size, cur);
  cur += sizeof(code_size);
  Copy(program->code, cur, code_size);
  cur += code_size;
  Copy(strs, cur, sizeof(strs));
  cur += sizeof(strs);
  WriteBE(strs_size, cur);
  cur += sizeof(strs_size);
  Copy(program->strings, cur, strs_size);
  return size;
}
