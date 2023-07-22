#include "run.h"
#include "parse.h"
#include "compile.h"
#include "assemble.h"
#include "env.h"
#include "module.h"
#include "std.h"
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
  PrintVal(message, mem);
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

  Assemble(compiled.result, vm->chunk, &vm->mem);

  TakeOutGarbage(vm);

  Val result = RunChunk(vm, vm->chunk);
  return result;
}

static bool REPLCmd(char *text, VM *vm)
{
  if (StrEq(text, "@help")) {
    Print("Type any expression, and it will be evaluated.\n");
    Print("To quit, use Ctrl+D at an empty prompt, or Ctrl+C.\n");
    Print("Some commands to the VM are also supported:\n");
    Print("  @help      Prints this help\n");
    Print("  @reset     Resets the VM\n");
    Print("  @trace     Toggles instruction tracing\n");
    Print("  @env       Prints the current environment\n");
    Print("  @mem       Prints the current memory contents\n");
    Print("  @symbols   Prints the symbol table\n");
    Print("  @stack     Prints the value stack\n");
    Print("  @calls     Prints the call stack\n");
    Print("  @code      Disassembles the current chunk\n");
    Print("  @gc        Runs the garbage collector\n");
    return true;
  } else if (StrEq(text, "@reset")) {
    ResetVM(vm);
    return true;
  } else if (StrEq(text, "@env")) {
    PrintEnv(vm->env, &vm->mem);
    return true;
  } else if (StrEq(text, "@mem")) {
    PrintMem(&vm->mem);
    return true;
  } else if (StrEq(text, "@symbols")) {
    PrintSymbols(&vm->mem);
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
  } else if (StrEq(text, "@gc")) {
    u32 start = VecCount(vm->mem.values);
    TakeOutGarbage(vm);
    u32 collected = start - VecCount(vm->mem.values);
    Print("Collected ");
    PrintInt(collected);
    Print(" values\n");
    return true;
  }

  return false;
}

void REPL(Opts opts)
{
  VM vm;
  InitVM(&vm);
  Chunk chunk = GetStdChunk();
  vm.chunk = &chunk;
  vm.trace = opts.trace;

  HashMap modules;
  InitHashMap(&modules);
  FindModules(opts.module_path, &modules, &vm.mem);

  signal(SIGINT, OnSignal);
  RawConsole();

  PrintVersion();
  Print("Exit with Ctrl+C. Type \"@help\" for help.\n");

  u32 input_max = 1024;
  char text[input_max];
  text[0] = '\0';

  while (true) {
    u32 text_len = StrLen(text);
    if (text_len > 0) Print(". ");
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

      if (IsTagged(expr, "import", &vm.mem)) {
        Val import = ListAt(expr, 1, &vm.mem);
        if (HashMapContains(&modules, import.as_i)) {
          Val module = (Val){.as_i = HashMapGet(&modules, import.as_i)};
          if (!Eq(module, SymbolFor("*loaded*"))) {
            HashMapSet(&modules, import.as_i, SymbolFor("*loaded*").as_i);
            Eval(module, &vm);
            expr = Parse(text, &vm.mem);
          }
        }
      }
      if (vm.error) continue;

      if (IsTagged(expr, "error", &vm.mem)) {
        if (Eq(Tail(expr, &vm.mem), SymbolFor("partial"))) {
          // partial parse, do nothing & fetch more input
          input[input_len] = '\n';
          input[input_len+1] = '\0';
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

void RunFile(Opts opts)
{
  VM vm;
  InitVM(&vm);
  Chunk chunk;
  InitChunk(&chunk);
  vm.chunk = &chunk;
  vm.trace = opts.trace;
  char *filename = opts.filename;

  if (SniffChunk(filename)) {
    if (ReadChunk(&chunk, filename)) {
      RunChunk(&vm, &chunk);
    } else {
      PrintEscape(IOFGRed);
      Print("Error loading chunk \"");
      Print(filename);
      Print("\"");
      PrintEscape(IOFGReset);
      Print("\n");
    }
    return;
  }

  Val ast = LoadModule(filename, opts.module_path, &vm.mem);
  if (IsTagged(ast, "error", &vm.mem)) {
    Val type = ListAt(ast, 1, &vm.mem);
    if (Eq(type, SymbolFor("parse"))) {
      PrintParseError(ast, &vm.mem);
    } else if (Eq(type, SymbolFor("load"))) {
      PrintLoadError(ast, &vm.mem);
    }
  } else {
    if (opts.print_ast) PrintAST(ast, &vm.mem);
    Eval(ast, &vm);
  }
}

void CompileFile(char *filename, char *module_path)
{
  Mem mem;
  InitMem(&mem);

  Val ast = LoadModule(filename, module_path, &mem);
  PrintAST(ast, &mem);

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
      Assemble(compiled.result, &chunk, &mem);
      WriteChunk(&chunk, ReplaceExtension(filename, "tape"));
    } else {
      PrintCompileError(compiled, &mem);
    }
  }
}
