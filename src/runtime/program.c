#include "runtime/program.h"
#include "runtime/ops.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/vec.h"

Program *NewProgram(void)
{
  Program *program = malloc(sizeof(Program));
  program->code = 0;
  InitSourceMap(&program->srcmap);
  program->strings = 0;
  return program;
}

void FreeProgram(Program *program)
{
  if (program->code) FreeVec(program->code);
  DestroySourceMap(&program->srcmap);
  if (program->strings) FreeVec(program->strings);
  free(program);
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

#ifdef DEBUG
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
#endif
