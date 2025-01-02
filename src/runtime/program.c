#include "runtime/program.h"
#include "univ/file.h"
#include "univ/vec.h"
#include "univ/iff.h"
#include "univ/str.h"
#include "univ/compress.h"

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
  IFFChunk *code, *strs, *form;
  u8 *compressed;
  u32 length;

  length = Compress(program->code, VecCount(program->code), &compressed, 8);
  code = NewIFFChunk('CODE', compressed, length);
  free(compressed);

  length = Compress(program->strings, VecCount(program->strings), &compressed, 8);
  strs = NewIFFChunk('STRS', compressed, length);
  free(compressed);

  form = NewIFFForm('TAPE');
  form = IFFAppendChunk(form, code);
  form = IFFAppendChunk(form, strs);
  return form;
}

Program *DeserializeProgram(IFFChunk *chunk)
{
  Program *program;
  IFFChunk *code, *strs;
  u32 size;
  u8 *data;

  if (IFFFormType(chunk) != 'TAPE') return 0;

  code = IFFGetField(chunk, 0);
  if (!code || IFFChunkType(code) != 'CODE') return 0;
  strs = IFFGetField(chunk, 1);
  if (!strs || IFFChunkType(strs) != 'STRS') return 0;

  program = NewProgram();
  size = Decompress(IFFData(code), IFFDataSize(code), &data, 8);
  GrowVec(program->code, size);
  Copy(data, program->code, size);
  free(data);
  size = Decompress(IFFData(strs), IFFDataSize(strs), &data, 8);
  GrowVec(program->strings, size);
  Copy(data, program->strings, size);
  free(data);
  return program;
}

Program *ReadProgramFile(char *filename)
{
  IFFChunk *data = ReadFile(filename);
  if (!data) return 0;
  return DeserializeProgram(data);
}

void WriteProgramFile(Program *program, char *filename)
{
  IFFChunk *chunk = SerializeProgram(program);
  WriteFile(chunk, IFFChunkSize(chunk), filename);
  free(chunk);
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
