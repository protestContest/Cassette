#include "module.h"
#include "env.h"
#include "reader.h"
#include "eval.h"
#include "mem.h"
#include "printer.h"
#include <dirent.h>

Module *LoadFile(char *path)
{
  if (DEBUG_MODULE) fprintf(stderr, "Loading file %s\n", path);

  Reader *r = NewReader();
  ReadFile(r, path);

  if (r->status == PARSE_INCOMPLETE) {
    ParseError(r, "Unexpected end of file");
  }

  if (r->status == PARSE_ERROR) {
    PrintReaderError(r);
    return NULL;
  }

  Val env = ExtendEnv(InitialEnv(), nil, nil);
  Val body = Tail(r->ast);
  while (!IsNil(body)) {
    Val exp = Head(body);
    if (IsTagged(exp, "module")) {
      EvalResult result = EvalModule(Tail(exp), env);
      if (result.status != EVAL_OK) {
        PrintEvalError(result);
        return NULL;
      }
    }
    body = Tail(body);
  }

  Module *mod = malloc(sizeof(Module));
  mod->path = path;
  mod->defs = DictGet(Head(GlobalEnv(env)), SymbolFor("MODULES"));
  return mod;
}

EvalResult RunFile(char *path, Val env)
{
  if (DEBUG_MODULE) fprintf(stderr, "Running file %s\n", path);

  Reader *r = NewReader();
  ReadFile(r, path);

  if (r->status == PARSE_INCOMPLETE) {
    ParseError(r, "Unexpected end of file");
  }

  if (r->status == PARSE_ERROR) {
    PrintReaderError(r);
    return RuntimeError("ParseError");
  }

  return Eval(r->ast, env);
}

Val RunProject(char *main_file)
{
  Module *mods = LoadModules(".", main_file);

  Val modules = MakeDict(nil, nil);

  while (mods != NULL) {
    DictMerge(mods->defs, modules);
    mods = mods->next;
  }

  Val env = InitialEnv();
  Define(MakeSymbol("MODULES"), modules, env);

  if (main_file == NULL) return env;

  EvalResult result = RunFile(main_file, env);
  if (result.status != EVAL_OK) {
    PrintEvalError(result);
  }
  return env;
}

Module *AppendModules(Module *mods1, Module *mods2)
{
  if (mods1 == NULL) return mods2;
  while (mods1->next != NULL) mods1 = mods1->next;
  mods1->next = mods2;
  return mods1;
}

Module *LoadModules(char *name, char *main_file)
{
  Module *mods = NULL;

  DIR *dir = opendir(name);
  if (dir == NULL) {
    fprintf(stderr, "Bad directory name: %s\n", name);
    return NULL;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      char path[1024];
      if (entry->d_name[0] == '.' && (!main_file || strcmp(entry->d_name, main_file) != 0)) continue;
      snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
      Module *sub_mods = LoadModules(path, "");
      if (sub_mods) mods = AppendModules(mods, sub_mods);
    } else if (entry->d_type == DT_REG
        && entry->d_namlen > 4
        && (!main_file || strcmp(entry->d_name, main_file) != 0)) {
      if (strcmp(entry->d_name + (entry->d_namlen - 4), ".rye") == 0) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
        Module *mod = LoadFile(entry->d_name);
        if (mod) {
          mod->next = mods;
          mods = mod;
        }
      }
    }
  }

  return mods;
}
