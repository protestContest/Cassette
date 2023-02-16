#include "native.h"
#include "base.h"
#include "mem.h"
#include "vm.h"
#include "env.h"
#include "printer.h"

static void InitNativeMap(NativeMap *map)
{
  map->count = 0;
  map->capacity = 64;
  map->items = malloc(sizeof(NativeMapItem)*map->capacity);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].name = nil;
  }
}

static void ResizeNativeMap(NativeMap *map, u32 capacity)
{
  map->capacity = capacity;
  map->items = realloc(map->items, capacity*sizeof(NativeMapItem));
}

void NativeMapPut(NativeMap *map, Val key, NativeFn impl)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      map->items[index].impl = impl;
      return;
    }
    index = (index + 1) % map->capacity;
  }

  map->items[index].name = key;
  map->items[index].impl = impl;
  map->count++;

  if (map->count > 0.8*map->capacity) {
    ResizeNativeMap(map, 2*map->capacity);
  }
}

static NativeFn NativeMapGet(NativeMap *map, Val key)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      return map->items[index].impl;
    }
  }

  return NULL;
}

typedef struct {
  char *name;
  NativeFn impl;
} NativeDef;

static void NativePrint(VM *vm, u32 num_args);
static void NativeEq(VM *vm, u32 num_args);
static void NativeReverse(VM *vm, u32 num_args);
static void NativeHead(VM *vm, u32 num_args);
static void NativeTail(VM *vm, u32 num_args);

static NativeDef natives[] = {
  { "print",    &NativePrint    },
  { "eq?",      &NativeEq       },
  { "reverse",  &NativeReverse  },
  { "head",     &NativeHead     },
  { "tail",     &NativeTail     },
};

void DefineNatives(VM *vm)
{
  // TODO: add natives under a module name parameter

  InitNativeMap(&vm->natives);

  for (u32 i = 0; i < ArrayCount(natives); i++) {
    Val name = MakeSymbol(&vm->chunk->strings, natives[i].name);
    NativeMapPut(&vm->natives, name, natives[i].impl);
    Define(vm, name, MakePair(&vm->heap, SymbolFor("native"), name), vm->env);
  }
}

void DoNative(VM *vm, Val name, u32 num_args)
{
  NativeFn fn = NativeMapGet(&vm->natives, name);
  if (fn == NULL) {
    RuntimeError(vm, "Undefined native function \"%s\"", SymbolName(&vm->chunk->strings, name));
    return;
  }

  fn(vm, num_args);
}

/* Native bindings */

static void NativePrint(VM *vm, u32 num_args)
{
  for (u32 i = 0; i < num_args; i++) {
    Val val = StackPop(vm);
    PrintVal(vm->heap, &vm->chunk->strings, val);
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
  StackPush(vm, Reverse(&vm->heap, list));
}

static void NativeHead(VM *vm, u32 num_args)
{
  Val pair = StackPop(vm);
  StackPush(vm, Head(vm->heap, pair));
}

static void NativeTail(VM *vm, u32 num_args)
{
  Val pair = StackPop(vm);
  StackPush(vm, Tail(vm->heap, pair));
}
