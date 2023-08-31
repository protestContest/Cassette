#include "io.h"
#include "list.h"

Val IOPrint(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val item = TupleGet(args, 0, mem);
  if (IsNum(item)) {
    Inspect(item, mem);
    Print("\n");
  } else if (IsSym(item)) {
    Print(SymbolName(item, mem));
    Print("\n");
  } else if (IsBinary(item, mem)) {
    PrintN(BinaryData(item, mem), BinaryLength(item, mem));
    Print("\n");
  } else if (IsPair(item)) {
    Val bin = ListToBin(vm, args);
    if (vm->error) return bin;
    PrintN(BinaryData(bin, mem), BinaryLength(bin, mem));
    Print("\n");
  } else {
    vm->error = TypeError;
    return item;
  }
  return SymbolFor("ok");
}

Val IOInspect(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val item = TupleGet(args, 0, mem);
  Inspect(item, mem);
  Print("\n");
  return SymbolFor("ok");
}

Val IOOpen(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  Val arg = TupleGet(args, 0, mem);

  if (!IsBinary(arg, mem)) {
    return ErrorResult("bad_arg", mem);
  }

  char path[BinaryLength(arg, mem) + 1];
  Copy(BinaryData(arg, mem), path, BinaryLength(arg, mem));
  path[BinaryLength(arg, mem)] = '\0';

  i32 file = Open(path);
  if (file < 0) {
    return ErrorResult("file_error", mem);
  }

  return OkResult(IntVal(file), mem);
}

Val IORead(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) return ErrorResult("arg_num", mem);

  Val file = TupleGet(args, 0, mem);
  Val length = TupleGet(args, 1, mem);
  if (!IsInt(file)) return ErrorResult("bad_arg", mem);
  if (!IsInt(length)) return ErrorResult("bad_arg", mem);

  Val binary = MakeBinary(RawInt(length), mem);
  if (Read(RawInt(file), BinaryData(binary, mem), RawInt(length))) {
    return OkResult(binary, mem);
  } else {
    return ErrorResult("io_error", mem);
  }
}

Val IOWrite(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) == 1) return IOPrint(vm, args);
  if (TupleLength(args, mem) != 2) return ErrorResult("arg_num", mem);

  Val file = TupleGet(args, 0, mem);
  Val data = TupleGet(args, 1, mem);
  if (!IsInt(file)) return ErrorResult("bad_arg", mem);
  if (!IsBinary(data, mem)) return ErrorResult("bad_arg", mem);

  if (Write(RawInt(file), BinaryData(data, mem), BinaryLength(data, mem))) {
    return OkResult(IntVal(BinaryLength(data, mem)), mem);
  } else {
    return ErrorResult("io_error", mem);
  }
}

Val IOReadFile(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) return ErrorResult("arg_num", mem);

  Val filename = TupleGet(args, 0, mem);
  Val length = TupleGet(args, 1, mem);
  if (!IsBinary(filename, mem)) return ErrorResult("bad_arg", mem);
  if (!IsInt(length)) return ErrorResult("bad_arg", mem);

  char path[BinaryLength(filename, mem) + 1];
  Copy(BinaryData(filename, mem), path, BinaryLength(filename, mem));
  path[BinaryLength(filename, mem)] = '\0';

  int file = Open(path);
  if (file < 0) return ErrorResult("io_error", mem);

  Val binary = MakeBinary(FileSize(file), mem);
  if (Read(file, BinaryData(binary, mem), BinaryLength(binary, mem))) {
    return OkResult(binary, mem);
  } else {
    return ErrorResult("io_error", mem);
  }
}

Val IOWriteFile(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) return ErrorResult("arg_num", mem);

  Val filename = TupleGet(args, 0, mem);
  Val data = TupleGet(args, 1, mem);
  if (!IsBinary(filename, mem)) return ErrorResult("bad_arg", mem);
  if (!IsBinary(data, mem)) return ErrorResult("bad_arg", mem);

  char path[BinaryLength(filename, mem) + 1];
  Copy(BinaryData(filename, mem), path, BinaryLength(filename, mem));
  path[BinaryLength(filename, mem)] = '\0';

  if (WriteFile(path, BinaryData(data, mem), BinaryLength(data, mem))) {
    return OkResult(IntVal(BinaryLength(data, mem)), mem);
  } else {
    return ErrorResult("io_error", mem);
  }
}
