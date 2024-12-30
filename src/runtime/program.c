#include "runtime/program.h"
#include "univ/file.h"
#include "univ/vec.h"
#include "univ/iff.h"
#include "univ/str.h"

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

IFFChunk *SerializeProgram(Program *program)
{
  IFFChunk *code = NewIFFChunk('CODE', program->code, VecCount(program->code));
  IFFChunk *strs = NewIFFChunk('STRS', program->strings, VecCount(program->strings));
  IFFChunk *form = NewIFFForm('TAPE');
  form = IFFAppendChunk(form, code);
  form = IFFAppendChunk(form, strs);
  return form;
}

Program *DeserializeProgram(IFFChunk *chunk)
{
  Program *program;
  IFFChunk *code, *strs;
  code = IFFGetFormField(chunk, 'TAPE', 'CODE');
  strs = IFFGetFormField(chunk, 'TAPE', 'STRS');

  program = NewProgram();
  GrowVec(program->code, IFFDataSize(code));
  Copy(IFFData(code), program->code, IFFDataSize(code));
  GrowVec(program->strings, IFFDataSize(strs));
  Copy(IFFData(strs), program->strings, IFFDataSize(strs));
  return program;
}

Program *ReadProgramFile(char *filename)
{
  IFFChunk *data = ReadFile(filename);
  if (!data) return false;
  if (IFFFormType(data) != 'TAPE') return 0;
  return DeserializeProgram(data);
}

#ifdef DEBUG
#include "runtime/ops.h"
#include "univ/math.h"

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
