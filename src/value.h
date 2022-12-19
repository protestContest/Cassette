#pragma once

/*

Values are NaN-boxed in 32-bit floats. Besides floats, there are four other
"immediate" types: symbols, lists, indexes, and objects.

  Float format: SEEEEEEE EMMMMMMM MMMMMMMM MMMMMMMM
  qNaN:         ?1111111 111????? ???????? ????????

The sign bit and bit 21 form a type tag, with the remaining 20 bits used as
indexes into the appropriate memory data structure.

  Symbol:       0_______ ___0nnnn nnnnnnnn nnnnnnnn   (n = index into symbol table)
  Pair:         0_______ ___1nnnn nnnnnnnn nnnnnnnn   (n = index into pairs table)
  Index:        1_______ ___0nnnn nnnnnnnn nnnnnnnn   (n = unsigned index value)
  Object:       1_______ ___1nnnn nnnnnnnn nnnnnnnn   (n = heap index)

The symbol table stores an object pointer to the symbol's binary representation.
The values nil, true, and false are the first three symbols.

The pairs table stores a pair's head and tail. An index is simply an immediate
unsigned integer. An object's index is a word-offset into heap memory.

Objects on the heap start with a 32-bit header. The header indicates the type
with the 3 high bits, and the low 29 bits encode the object size in bytes.

  Binary:       000nnnnn nnnnnnnn nnnnnnnn nnnnnnnn
  Closure:      001nnnnn nnnnnnnn nnnnnnnn nnnnnnnn
  Tuple:        010nnnnn nnnnnnnn nnnnnnnn nnnnnnnn
  Dict:         011nnnnn nnnnnnnn nnnnnnnn nnnnnnnn

*/

#include "hash.h"
#include "vm.h"

typedef enum {
  NUMBER,
  SYMBOL,
  PAIR,
  INDEX,
  OBJECT
} ValType;

typedef enum {
  BINARY,
  FUNCTION,
  TUPLE,
  DICT
} ObjType;

extern Value nil_val;
extern Value true_val;
extern Value false_val;

#define nan_mask    0x7FE00000
#define type_mask   (nan_mask | 0x80100000)
#define sym_mask    (nan_mask | 0x00000000)
#define pair_mask   (nan_mask | 0x00100000)
#define idx_mask    (nan_mask | 0x80000000)
#define obj_mask    (nan_mask | 0x80100000)

#define IsNumber(v) (((v) & nan_mask) != nan_mask)
#define IsSymbol(v) (((v) & type_mask) == sym_mask)
#define IsPair(v)   (((v) & type_mask) == pair_mask)
#define IsIndex(v)  (((v) & type_mask) == idx_mask)
#define IsObject(v) (((v) & type_mask) == obj_mask)

#define obj_type_mask   0xE0000000
#define bin_mask        0x00000000
#define fun_mask        0x20000000
#define tpl_mask        0x40000000
#define dct_mask        0x60000000
#define IsBinary(v)     (IsObject(v) && (*HeapRef(v) & obj_type_mask) == bin_mask)
#define IsFunction(v)   (IsObject(v) && (*HeapRef(v) & obj_type_mask) == fun_mask)
#define IsTuple(v)      (IsObject(v) && (*HeapRef(v) & obj_type_mask) == tpl_mask)
#define IsDict(v)       (IsObject(v) && (*HeapRef(v) & obj_type_mask) == dct_mask)

#define IsNil(v)    ((v) == nil_val)

#define NumberVal(n)  (Value)(n)
#define SymbolVal(s)  (Value)(((s) & (~type_mask)) | sym_mask)
#define PairVal(p)    (Value)(((p) & (~type_mask)) | pair_mask)
#define IndexVal(i)   (Value)(((i) & (~type_mask)) | idx_mask)
#define ObjectVal(o)  (Value)(((o) & (~type_mask)) | obj_mask)
#define RawVal(v)     (IsNumber(v) ? (v) : (v) & (~type_mask))

#define header_mask         0xE0000000
#define BinaryHeader(size)  ((size) & (~header_mask) | bin_mask)
#define HeaderValue(hdr)    ((hdr) & (~header_mask))

#define IsEqual(a, b)       (RawVal(a) == RawVal(b))
#define IsLessThan(a, b)    (RawVal(a) < RawVal(b))
#define Min(a, b)           (IsLessThan(a, b) ? a : b)
#define Incr(n)             IndexVal(RawVal(n) + 1)
#define Decr(n)             IndexVal(RawVal(n) - 1)

#define ValueHash(v)  Hash(&key, sizeof(Value))

ValType TypeOf(Value value);
ObjType ObjTypeOf(Value value);

u32 ObjectSize(Value value);

Value MakeBinary(char *src, u32 start, u32 end);
Value CreateBinary(char *src);
Value CopySlice(Value text, Value start, Value end);
#define BinarySize(binary)  ObjectSize(binary)
#define BinaryData(value)   ((char *)(HeapRef(value) + 1))
