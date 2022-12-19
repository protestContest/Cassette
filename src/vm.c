#include "vm.h"
#include "vector.h"
#include "printer.h"
#include "dict.h"

typedef struct VM {
  Vector pairs;
  u32 pair_next;
  Vector heap;
  u32 heap_next;
  Dict symbols;
  u32 longest_sym;
} VM;

#define INITIAL_NUM_PAIRS 32
#define INITIAL_HEAP_SIZE 32

static VM vm;

Value nil_val = 0;
Value true_val = 0;
Value false_val = 0;

void InitVM(void)
{
  InitVector(&vm.pairs, INITIAL_NUM_PAIRS*2);
  vm.pair_next = 0;
  InitVector(&vm.heap, INITIAL_HEAP_SIZE);
  vm.heap_next = 0;

  InitDict(&vm.symbols);

  nil_val = CreateSymbol("nil");
  true_val = CreateSymbol("true");
  false_val = CreateSymbol("false");
}

Value MakePair(Value head, Value tail)
{
  u32 index = PairVal(vm.pair_next);
  VectorSet(&vm.pairs, vm.pair_next++, head);
  VectorSet(&vm.pairs, vm.pair_next++, tail);
  return index;
}

Value Head(Value pair)
{
  return VectorGet(&vm.pairs, RawVal(pair));
}

Value Tail(Value pair)
{
  return VectorGet(&vm.pairs, RawVal(pair) + 1);
}

void SetHead(Value pair, Value head)
{
  VectorSet(&vm.pairs, RawVal(pair), head);
}

void SetTail(Value pair, Value tail)
{
  VectorSet(&vm.pairs, RawVal(pair) + 1, tail);
}

ObjHeader *HeapRef(Value index)
{
  return (ObjHeader*)VectorRef(&vm.heap, RawVal(index));
}

Value Allocate(ObjHeader header)
{
  u32 bytes = HeaderValue(header);
  u32 size = (bytes - 1) / sizeof(Value) + 1 + 1;

  VectorGrow(&vm.heap, vm.heap_next + size);
  VectorSet(&vm.heap, vm.heap_next, header);

  Value index = ObjectVal(vm.heap_next);
  vm.heap_next += size;

  return index;
}

void DumpPairs(void)
{
  const u32 min_len = 8;

  u32 sym_len = LongestSymLength();
  if (sym_len < min_len) sym_len = min_len;

  if (vm.pair_next == 0) {
    EmptyTable("⚭ Pairs");
    return;
  }

  Table *table = BeginTable("⚭ Pairs", 3, 4, sym_len, sym_len);
  for (u32 i = 0; i < vm.pair_next; i++) {
    Value head = VectorGet(&vm.pairs, i);
    Value tail = VectorGet(&vm.pairs, i+1);
    TableRow(table, IndexVal(i), head, tail);
  }
  EndTable(table);
}

void DumpHeap(void)
{
  u8 *data = (u8*)VectorRef(&vm.heap, 0);
  HexDump(data, vm.heap_next*4, "⨳ Heap");
}

Value MakeSymbolFrom(Value text, Value start, Value end)
{
  u32 len = RawVal(end) - RawVal(start);
  char *data = BinaryData(text) + RawVal(start);
  Value symbol = SymbolVal(Hash(data, len));

  if (DictHasKey(&vm.symbols, symbol)) {
    return symbol;
  } else {
    Value binary = CopySlice(text, start, end);
    DictPut(&vm.symbols, symbol, binary);
    if (len > vm.longest_sym) {
      vm.longest_sym = len;
    }

    return symbol;
  }
}

Value CreateSymbol(char *src)
{
  u32 len = strlen(src);
  Value symbol = SymbolVal(Hash(src, len));

  if (DictHasKey(&vm.symbols, symbol)) {
    return symbol;
  } else {
    Value binary = CreateBinary(src);
    DictPut(&vm.symbols, symbol, binary);
    if (len > vm.longest_sym) {
      vm.longest_sym = len;
    }

    return symbol;
  }
}

Value GetSymbol(char *src)
{
  u32 len = strlen(src);
  Value symbol = SymbolVal(Hash(src, len));
  if (DictHasKey(&vm.symbols, symbol)) {
    return symbol;
  } else {
    return nil_val;
  }
}

u32 LongestSymLength(void)
{
  return vm.longest_sym;
}

Value SymbolName(Value sym)
{
  Value binary = DictGet(&vm.symbols, sym);
  if (IsNil(binary)) {
    Error("No such symbol 0x%0X", sym);
  }

  return binary;
}

void DumpSymbols(void)
{
  u32 table_width = LongestSymLength();
  DumpDict(&vm.symbols, "★ Symbols", table_width);
}

