#include "runtime/program.h"
#include "univ/file.h"
#include "univ/vec.h"
#include "univ/iff.h"
#include "univ/str.h"
#include "univ/compress.h"
#include "univ/math.h"
#include "compile/opts.h"

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
  IFFChunk *code, *vers, *strs, *form;
  u8 *compressed;
  u32 length;
  u32 version[2];

  version[0] = ByteSwap(VERSION_MAJOR);
  version[1] = ByteSwap(VERSION_MINOR);

  vers = NewIFFChunk('VERS', version, ArrayCount(version)*sizeof(*version));

  length = Compress(program->code, VecCount(program->code), &compressed, 8);
  code = NewIFFChunk('CODE', compressed, length);
  free(compressed);

  length = Compress(program->strings, VecCount(program->strings), &compressed, 8);
  strs = NewIFFChunk('STRS', compressed, length);
  free(compressed);

  form = NewIFFForm('TAPE');
  form = IFFAppendChunk(form, vers);
  form = IFFAppendChunk(form, code);
  form = IFFAppendChunk(form, strs);
  return form;
}

static Error *BadProgramFile(void)
{
  return NewError("Corrupt program file", 0, -1, 0);
}

static Error *UnsupportedVersion(void)
{
  return NewError("Unsupported version", 0, -1, 0);
}

Error *DeserializeProgram(IFFChunk *chunk, Program **result)
{
  Program *program;
  IFFChunk *vers, *code, *strs;
  u32 size;
  u8 *data;
  u32 vMajor, vMinor;

  if (IFFFormType(chunk) != 'TAPE') return BadProgramFile();

  vers = IFFGetField(chunk, 0);
  if (!vers || IFFChunkType(vers) != 'VERS') return BadProgramFile();
  vMajor = ByteSwap(((u32*)IFFData(chunk))[0]);
  if (vMajor > VERSION_MAJOR) UnsupportedVersion();
  vMinor = ByteSwap(((u32*)IFFData(chunk))[1]);
  if (vMinor > VERSION_MINOR) UnsupportedVersion();
  code = IFFGetField(chunk, 1);
  if (!code || IFFChunkType(code) != 'CODE') BadProgramFile();
  strs = IFFGetField(chunk, 2);
  if (!strs || IFFChunkType(strs) != 'STRS') BadProgramFile();

  program = NewProgram();
  size = Decompress(IFFData(code), IFFDataSize(code), &data, 8);
  GrowVec(program->code, size);
  Copy(data, program->code, size);
  free(data);
  size = Decompress(IFFData(strs), IFFDataSize(strs), &data, 8);
  GrowVec(program->strings, size);
  Copy(data, program->strings, size);
  free(data);

  *result = program;
  return 0;
}

Error *ReadProgramFile(char *filename, Program **program)
{
  IFFChunk *data = ReadFile(filename);
  Error *error;
  if (!data) NewError("Couldn't read file", filename, 0, 0);
  error = DeserializeProgram(data, program);
  if (error) {
    error->filename = filename;
  }
  return error;
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
