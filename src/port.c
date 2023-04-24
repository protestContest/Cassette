#include "port.h"
#include "mem.h"
#include "eval.h"
#include <univ/base.h>
#include <univ/window.h>

static u32 PortKey(VM *vm, Val type, Val name)
{
  char *type_name = SymbolName(vm->mem, type);
  u32 key = Hash(type_name, StrLen(type_name));

  if (!IsNil(name)) {
    char name_str[BinaryLength(vm->mem, name) + 1];
    BinaryToString(vm->mem, name, name_str);
    key = AddHash(key, name_str, BinaryLength(vm->mem, name));
  }

  return key;
}

static Val MakePort(VM *vm, Val type, Val name, Val value)
{
  Val port = MakeTuple(vm->mem, 3);
  TupleSet(vm->mem, port, 0, type);
  TupleSet(vm->mem, port, 1, name);
  TupleSet(vm->mem, port, 2, value);

  MapSet(&vm->ports, PortKey(vm, type, name), port.as_v);

  return port;
}

static Val PortType(Mem *mem, Val port)
{
  return TupleAt(mem, port, 0);
}

static Val PortName(Mem *mem, Val port)
{
  return TupleAt(mem, port, 1);
}

static Val PortValue(Mem *mem, Val port)
{
  return TupleAt(mem, port, 2);
}

static Buf *PortBuffer(VM *vm, Val port)
{
  return &vm->buffers[RawInt(PortValue(vm->mem, port))];
}

static Val OpenFilePort(VM *vm, Val name, Val opts)
{
  char path[BinaryLength(vm->mem, name) + 1];
  BinaryToString(vm->mem, name, path);

  Buf buffer = Open(path);
  if (buffer.error) {
    return RuntimeError("Could not open file", name, vm->mem);
  }

  Val buf_num = IntVal(VecCount(vm->buffers));
  VecPush(vm->buffers, buffer);

  return MakePort(vm, SymbolFor("file"), name, buf_num);
}

static Val OpenConsolePort(VM *vm)
{
  if (HasPort(vm, SymbolFor("console"), nil)) {
    return GetPort(vm, SymbolFor("console"), nil);
  }

  Val buf_num = IntVal(VecCount(vm->buffers));
  VecPush(vm->buffers, *output);

  return MakePort(vm, SymbolFor("console"), nil, buf_num);
}

static Val OpenWindowPort(VM *vm, Val name, Val opts)
{
  char title[BinaryLength(vm->mem, name) + 1];
  BinaryToString(vm->mem, name, title);

  u32 width = 400;
  u32 height = 300;
  if (!IsNil(opts)) {
    if (!IsDict(vm->mem, opts)) {
      return RuntimeError("Expected window arguments to be a dict", opts, vm->mem);
    }

    if (InDict(vm->mem, opts, SymbolFor("width"))) {
      width = RawInt(DictGet(vm->mem, opts, SymbolFor("width")));
    }
    if (InDict(vm->mem, opts, SymbolFor("height"))) {
      height = RawInt(DictGet(vm->mem, opts, SymbolFor("height")));
    }
  }

  Window *window = NewWindow(title, width, height);
  Val window_num = IntVal(VecCount(vm->windows));
  VecPush(vm->windows, window);
  WaitNextEvent(ActivateEvent);

  return MakePort(vm, SymbolFor("window"), name, window_num);
}

bool HasPort(VM *vm, Val type, Val name)
{
  return MapContains(&vm->ports, PortKey(vm, type, name));
}

Val GetPort(VM *vm, Val type, Val name)
{
  return ObjVal(MapGet(&vm->ports, PortKey(vm, type, name)));
}

Val OpenPort(VM *vm, Val type, Val name, Val opts)
{
  if (Eq(type, SymbolFor("console"))) {
    return OpenConsolePort(vm);
  }

  if (HasPort(vm, type, name)) {
    return GetPort(vm, type, name);
  }

  if (Eq(type, SymbolFor("file"))) {
    return OpenFilePort(vm, name, opts);
  } else if (Eq(type, SymbolFor("window"))) {
    return OpenWindowPort(vm, name, opts);
  }

  return nil;
}

static Val SendToConsole(VM *vm, Val message)
{
  if (!ValToString(vm->mem, message, output)) {
    return RuntimeError("Could not print", message, vm->mem);
  }

  return SymbolFor("ok");
}

static Val SendToWindow(VM *vm, Val port, Val message)
{
  Val value = TupleAt(vm->mem, port, 2);
  Window *window = vm->windows[RawInt(value)];

  Val cmd = ListAt(vm->mem, message, 0);
  Assert(Eq(cmd, SymbolFor("line")));

  Val x1 = ListAt(vm->mem, message, 1);
  Val y1 = ListAt(vm->mem, message, 2);
  Val x2 = ListAt(vm->mem, message, 3);
  Val y2 = ListAt(vm->mem, message, 4);

  DrawLine(&window->canvas, RawInt(x1), RawInt(y1), RawInt(x2), RawInt(y2));
  RedrawWindow(window);

  return SymbolFor("ok");
}

static Val SendToFile(VM *vm, Val port, Val message)
{
  Buf *buffer = PortBuffer(vm, port);

  if (!ValToString(vm->mem, message, buffer)) {
    return RuntimeError("Could not print", message, vm->mem);
  }

  Flush(buffer);

  return SymbolFor("ok");
}

Val SendPort(VM *vm, Val port, Val message)
{
  Val type = PortType(vm->mem, port);

  if (Eq(type, SymbolFor("console"))) {
    return SendToConsole(vm, message);
  } else if (Eq(type, SymbolFor("window"))) {
    return SendToWindow(vm, port, message);
  } else if (Eq(type, SymbolFor("file"))) {
    return SendToFile(vm, port, message);
  }

  return nil;
}

Val RecvPort(VM *vm, Val port)
{
  Val type = PortType(vm->mem, port);

  if (Eq(type, SymbolFor("file"))) {

  }
  // TODO: Implement
  return nil;
}
