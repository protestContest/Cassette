#include "run.h"
#include "parse.h"
#include "compile.h"
#include "assemble.h"
#include "env.h"
#include "module.h"
#include <signal.h>

static void OnSignal(int sig)
{
  Exit();
}

void PrintVersion(void)
{
  Print("Cassette v");
  Print(VERSION);
  Print("\n");
}

static void PrintParseError(Val error, Mem *mem)
{
  Val message = ListAt(error, 2, mem);
  Val token = ListAt(error, 3, mem);
  Val text = ListAt(error, 4, mem);
  u32 pos = RawInt(ListAt(token, 1, mem));
  u32 length = RawInt(ListAt(token, 2, mem));
  u32 line = RawInt(ListAt(token, 3, mem));
  u32 col = RawInt(ListAt(token, 4, mem));

  PrintEscape(IOFGRed);
  Print("Parse error [");
  PrintInt(line);
  Print(":");
  PrintInt(col);
  Print("]: ");
  InspectVal(message, mem);
  Print("\n");
  PrintSourceContext(BinaryData(text, mem), line, 3, pos, length);
  PrintEscape(IOFGReset);
  Print("\n");
}

static void PrintLoadError(Val error, Mem *mem)
{
  Val filename = ListAt(error, 2, mem);
  PrintEscape(IOFGRed);
  Print("Error loading module \"");
  PrintVal(filename, mem);
  Print("\"");
  PrintEscape(IOFGReset);
  Print("\n");
}

static void PrintCompileError(CompileResult error, Mem *mem)
{
  PrintEscape(IOFGRed);
  Print("Compile error: ");
  Print(error.message);
  PrintEscape(IOFGReset);
  Print("\n");
}

Val Eval(Val ast, VM *vm)
{
  CompileResult compiled = Compile(ast, &vm->mem);
  if (!compiled.ok) {
    PrintCompileError(compiled, &vm->mem);
    vm->error = true;
    return nil;
  }

  Seq code = Preserving(RegEnv, compiled.result, MakeSeq(RegEnv, 0, nil), &vm->mem);
  Assemble(code, vm->chunk, &vm->mem);

  Val result = RunChunk(vm, vm->chunk);
  return result;
}

static bool REPLCmd(char *text, VM *vm)
{
  if (StrEq(text, "@reset\n")) {
    Chunk *chunk = vm->chunk;
    DestroyVM(vm);
    DestroyChunk(chunk);
    InitChunk(chunk);
    InitVM(vm);
    vm->chunk = chunk;
    return true;
  } else if (StrEq(text, "@env")) {
    PrintEnv(vm->env, &vm->mem);
    return true;
  } else if (StrEq(text, "@mem")) {
    PrintMem(&vm->mem);
    return true;
  } else if (StrEq(text, "@stack")) {
    PrintStack(vm);
    return true;
  } else if (StrEq(text, "@calls")) {
    PrintCallStack(vm);
    return true;
  } else if (StrEq(text, "@code")) {
    Disassemble(vm->chunk);
    return true;
  } else if (StrEq(text, "@trace")) {
    vm->trace = !vm->trace;
    if (vm->trace) Print("Trace on\n");
    else Print("Trace off\n");
    return true;
  }

  return false;
}

void REPL(void)
{
  signal(SIGINT, OnSignal);
  RawConsole();

  PrintVersion();

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

    // add input to end of text
    char *input = text + text_len;
    ReadLine(input, input_max - text_len);
    u32 input_len = StrLen(input);

    // exit on Ctrl+D from a blank prompt
    if (*text == 0x04) break;

    // clear prompt on Ctrl+D
    if (input[input_len-1] == 0x04) {
      Print("\n");
      text[0] = '\0';
      continue;
    }

    Print("\n");

    if (REPLCmd(text, &vm)) {
      text[0] = '\0';
    } else {
      Val expr = Parse(text, &vm.mem);

      if (IsTagged(expr, "error", &vm.mem)) {
        if (Eq(Tail(expr, &vm.mem), SymbolFor("partial"))) {
          // partial parse, do nothing & fetch more input
        } else {
          // real error
          PrintParseError(expr, &vm.mem);
          text[0] = '\0';
        }
      } else {
        Val result = Eval(expr, &vm);
        if (!vm.error) {
          Print("=> ");
          InspectVal(result, &vm.mem);
          Print("\n");
        }
        text[0] = '\0';
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
  vm.trace = true;

  if (SniffFile(filename, 0xCA55E77E)) {
    if (ReadChunk(&chunk, filename)) {
      RunChunk(&vm, &chunk);
    } else {
      PrintEscape(IOFGRed);
      Print("Error loading module \"");
      Print(filename);
      Print("\"");
      PrintEscape(IOFGReset);
      Print("\n");
    }
    return;
  }

  Val ast = LoadModule(filename, &vm.mem);
  if (IsTagged(ast, "error", &vm.mem)) {
    Val type = ListAt(ast, 1, &vm.mem);
    if (Eq(type, SymbolFor("parse"))) {
      PrintParseError(ast, &vm.mem);
    } else if (Eq(type, SymbolFor("load"))) {
      PrintLoadError(ast, &vm.mem);
    }
  } else {
    Eval(ast, &vm);
  }
}

void CompileFile(char *filename)
{
  Mem mem;
  InitMem(&mem);

  Val ast = LoadModule(filename, &mem);
  if (IsTagged(ast, "error", &mem)) {
    Val type = ListAt(ast, 1, &mem);
    if (Eq(type, SymbolFor("parse"))) {
      PrintParseError(ast, &mem);
    } else if (Eq(type, SymbolFor("load"))) {
      PrintLoadError(ast, &mem);
    }
  } else {
    CompileResult compiled = Compile(ast, &mem);
    if (compiled.ok) {
      Chunk chunk;
      InitChunk(&chunk);
      Seq code = Preserving(RegEnv, compiled.result, MakeSeq(RegEnv, 0, nil), &mem);
      Assemble(code, &chunk, &mem);
      WriteChunk(&chunk, ReplaceExtension(filename, "tape"));
    } else {
      PrintCompileError(compiled, &mem);
    }

  }
}
