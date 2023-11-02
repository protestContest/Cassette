#include "project.h"
#include "parse.h"
#include "env.h"
#include "module.h"
#include <stdio.h>
#include <stdlib.h>

#define Main      0x7FD99CB3

typedef struct {
  Val entry;
  HashMap modules;
  Mem mem;
  SymbolTable symbols;
} Project;

static void InitProject(Project *p);
static void DestroyProject(Project *p);
static bool ParseModules(char **filenames, u32 num_files, Project *project);
static Val ScanDependencies(Project *project);
static Chunk *CompileProject(Val build_list, Project *p);

Chunk *BuildProject(char **filenames, u32 num_files)
{
  Project p;
  Val build_list;
  Chunk *chunk = 0;

  InitProject(&p);

  MakeParseSyms(&p.symbols);

  if (!ParseModules(filenames, num_files, &p)) {
    /* print parse error */
    return 0;
  }

  build_list = ScanDependencies(&p);
  chunk = CompileProject(build_list, &p);

  Disassemble(chunk);

  DestroyProject(&p);
  return chunk;
}

static void InitProject(Project *p)
{
  p->entry = Nil;
  InitHashMap(&p->modules);
  InitMem(&p->mem, 1024);
  InitSymbolTable(&p->symbols);
}

static void DestroyProject(Project *p)
{
  p->entry = Nil;
  DestroyHashMap(&p->modules);
  DestroyMem(&p->mem);
  DestroySymbolTable(&p->symbols);
}

static bool ParseModules(char **filenames, u32 num_files, Project *project)
{
  u32 i;
  Parser p;

  InitParser(&p, &project->mem, &project->symbols);
  for (i = 0; i < num_files; i++) {
    Val name;
    Val mod = ParseModule(filenames[i], &p);

    if (mod == ParseError) return false;

    name = ModuleName(mod, &project->mem);

    if (i == 0) {
      if (name == Nil) {
        Assert(Main == Sym("*main*", &project->symbols));
        name = Main;
      }
     TupleSet(mod, 0, name, &project->mem);
      project->entry = name;
    }

    if (name != Nil) {
      HashMapSet(&project->modules, name, mod);
    }
  }

  return true;
}

static Val ScanDependencies(Project *project)
{
  Val queue = Pair(project->entry, Nil, &project->mem), build_list = Nil;

  while (queue != Nil) {
    Val cur = Head(queue, &project->mem);
    Val mod = HashMapGet(&project->modules, cur);
    Val imports = TupleGet(mod, 2, &project->mem);

    queue = Tail(queue, &project->mem);
    build_list = Pair(mod, build_list, &project->mem);

    while (imports != Nil) {
      Val import = Head(imports, &project->mem);
      if (!ListContains(build_list, import, &project->mem)) {
        queue = Pair(import, queue, &project->mem);
      }
      imports = Tail(imports, &project->mem);
    }
  }

  return build_list;
}

static Chunk *CompileProject(Val build_list, Project *p)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  Val module_env = CompileEnv(&p->mem, &p->symbols);
  Compiler c;
  u32 i = 0, patch;
  u32 num_modules = ListLength(build_list, &p->mem);

  InitChunk(chunk);
  InitCompiler(&c, &p->mem, &p->modules, chunk);

  /* env is {modules} -> {primitives} -> nil */
  module_env = ExtendEnv(module_env, MakeTuple(num_modules, &p->mem), &p->mem);
  PushByte(OpConst, 0, chunk);
  PushConst(IntVal(num_modules), 0, chunk);
  PushByte(OpTuple, 0, chunk);
  PushByte(OpExtend, 0, chunk);

  while (build_list != Nil) {
    CompileResult result;
    Val module = Head(build_list, &p->mem);
    build_list = Tail(build_list, &p->mem);

    printf("Compiling %s\n", SymbolName(ModuleName(module, &p->mem), &p->symbols));

    /* each module will create a lambda and define itself */
    result = CompileModule(module, module_env, i, &c);
    Define(ModuleName(module, &p->mem), i, module_env, &p->mem);

    if (!result.ok) {
      PrintCompileError(result, SymbolName(ModuleFile(module, &p->mem), &p->symbols));
    }
    i++;
  }

  patch = PushByte(OpLink, 0, chunk);
  PushByte(0, 0, chunk);
  PushByte(OpLookup, 0, chunk);
  PushConst(IntVal(0), 0, chunk);
  PushConst(IntVal(i-1), 0, chunk);
  PushByte(OpApply, 0, chunk);
  PushConst(IntVal(0), 0, chunk);
  PatchChunk(chunk, patch+1, IntVal(chunk->count - patch));
  PushByte(OpPop, 0, chunk);

  return chunk;
}
