#include "native.h"
#include "mem.h"
#include "vm.h"
#include "env.h"
#include "printer.h"

typedef struct {
  char *module;
  char *name;
  NativeFn impl;
} NativeDef;

static void NativePrint(VM *vm, u32 num_args);
static void NativeEq(VM *vm, u32 num_args);
static void NativeReverse(VM *vm, u32 num_args);
static void NativeHead(VM *vm, u32 num_args);
static void NativeTail(VM *vm, u32 num_args);

static NativeDef natives[] = {
  { "Kernel", "print",    &NativePrint    },
  { "Kernel", "eq?",      &NativeEq       },
  { "Kernel", "reverse",  &NativeReverse  },
  { "Kernel", "head",     &NativeHead     },
  { "Kernel", "tail",     &NativeTail     },
};

void DefineNatives(VM *vm)
{
  // TODO: add natives under a module name parameter

  InitNativeMap(&vm->natives);

  for (u32 i = 0; i < ArrayCount(natives); i++) {
    Val name = MakeSymbol(&vm->image->strings, natives[i].name);
    NativeMapPut(&vm->natives, name, natives[i].impl);
    Define(vm, name, MakePair(&vm->image->heap, SymbolFor("native"), name), vm->image->env);
  }
}

void DoNative(VM *vm, Val name, u32 num_args)
{
  NativeFn fn = NativeMapGet(&vm->natives, name);
  if (fn == NULL) {
    RuntimeError(vm, "Undefined native function \"%s\"", SymbolName(&vm->image->strings, name));
    return;
  }

  fn(vm, num_args);
}

/* Native bindings */

static void NativePrint(VM *vm, u32 num_args)
{
  for (u32 i = 0; i < num_args; i++) {
    Val val = StackPop(vm);
    PrintVal(vm->image->heap, &vm->image->strings, val);
  }
  printf("\n");
}

static void NativeEq(VM *vm, u32 num_args)
{
  bool result;

  if (num_args == 0) {
    result = false;
  } else if (num_args == 1) {
    result = true;
  } else {
    Val a = StackPop(vm);
    result = true;
    for (u32 i = 0; i < num_args - 1; i++) {
      Val b = StackPop(vm);
      if (!Eq(a, b)) {
        result = false;
        break;
      }
    }
  }

  if (result) {
    StackPush(vm, SymbolFor("true"));
  } else {
    StackPush(vm, SymbolFor("false"));
  }
}

static void NativeReverse(VM *vm, u32 num_args)
{
  Val list = StackPop(vm);
  StackPush(vm, Reverse(&vm->image->heap, list));
}

static void NativeHead(VM *vm, u32 num_args)
{
  Val pair = StackPop(vm);
  StackPush(vm, Head(vm->image->heap, pair));
}

static void NativeTail(VM *vm, u32 num_args)
{
  Val pair = StackPop(vm);
  StackPush(vm, Tail(vm->image->heap, pair));
}
