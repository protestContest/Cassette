#include "program.h"
#include "univ/str.h"
#include <univ/vec.h>

Program *NewProgram(void)
{
  Program *program = malloc(sizeof(Program));
  program->code = 0;
  program->symbols = 0;
  return program;
}

void FreeProgram(Program *program)
{
  if (program->code) FreeVec(program->code);
  if (program->symbols) FreeVec(program->symbols);
  free(program);
}

u32 SerializeProgram(Program *program, u8 **dst)
{
  u32 code_size = VecCount(program->code);
  u32 sym_size = VecCount(program->symbols);
  u32 size = code_size + sym_size + 2*sizeof(u32);
  u8 *cur;
  *dst = realloc(*dst, size);
  cur = *dst;
  Copy(&code_size, cur, sizeof(code_size));
  cur += sizeof(code_size);
  Copy(program->code, cur, code_size);
  cur += code_size;
  Copy(&sym_size, cur, sizeof(sym_size));
  cur += sizeof(sym_size);
  Copy(program->symbols, cur, VecCount(program->symbols));
  return size;
}
