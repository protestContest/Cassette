#include "run.h"
#include "parse.h"
#include "compile.h"
#include "assemble.h"
#include "env.h"
#include <signal.h>

static void OnSignal(int sig)
{
  Exit();
}

Val Eval(char *source, VM *vm)
{
  Val ast = Parse(source, &vm->mem);
  if (IsTagged(ast, "error", &vm->mem)) return ast;

  Seq code = Preserving(RegEnv, Compile(ast, &vm->mem), MakeSeq(RegEnv, 0, nil), &vm->mem);
  Assemble(code, vm->chunk, &vm->mem);

  Val result = RunChunk(vm, vm->chunk);
  return result;
}

void REPL(void)
{
  signal(SIGINT, OnSignal);
  RawConsole();

  VM vm;
  InitVM(&vm);
  Chunk chunk;
  InitChunk(&chunk);
  vm.chunk = &chunk;

  u32 input_max = 1024;
  char text[input_max];
  text[0] = '\0';

  while (true) {
    u32 text_len = StrLen(text);
    if (text_len > 0) Print("... ");
    else Print("> ");

    char *input = text + text_len;
    ReadLine(input, input_max - text_len);
    u32 input_len = StrLen(input);

    if (*text == 0x04) break;
    if (input[input_len-1] == 0x04) {
      Print("\n");
      text[0] = '\0';
      continue;
    }
    Print("\n");


    if (StrEq(text, "@reset\n")) {
      DestroyVM(&vm);
      DestroyChunk(&chunk);
      InitVM(&vm);
      InitChunk(&chunk);
      vm.chunk = &chunk;
      text[0] = '\0';
    } else if (StrEq(text, "@env")) {
      PrintEnv(vm.env, &vm.mem);
      text[0] = '\0';
    } else if (StrEq(text, "@mem")) {
      PrintMem(&vm.mem);
      text[0] = '\0';
    } else if (StrEq(text, "@stack")) {
      PrintStack(&vm);
      text[0] = '\0';
    } else if (StrEq(text, "@calls")) {
      PrintCallStack(&vm);
      text[0] = '\0';
    } else if (StrEq(text, "@code")) {
      Disassemble(vm.chunk);
      text[0] = '\0';
    } else if (StrEq(text, "@trace")) {
      vm.trace = !vm.trace;
      if (vm.trace) Print("Trace on\n");
      else Print("Trace off\n");
      text[0] = '\0';
    } else {
      Val result = Eval(text, &vm);

      if (!IsTagged(result, "error", &vm.mem) ||
          !Eq(Tail(result, &vm.mem), SymbolFor("partial"))) {
        text[0] = '\0';

        if (!vm.error) {
          PrintVal(result, &vm.mem);
          Print("\n");
        }
      }

    }
  }
}

void RunFile(char *filename)
{
  VM vm;
  InitVM(&vm);
  Chunk chunk;
  InitChunk(&chunk);
  vm.chunk = &chunk;


  char *source = (char*)ReadFile(filename);
  Eval(source, &vm);
}
