#pragma once

/*
Values are 32-bit NaN-boxed floats. When a value's floating point representation
is NaN, the remaining bits encode a type and value. The lower 20 bits (0–19) are
used for a raw value, while bits 20, 21, and 31 encode the type. If the floating
point representation isn't NaN, the value is a floating point number.

Floating format:  SEEEEEEE EMMMMMMM MMMMMMMM MMMMMMMM
NaN:              ?1111111 11?????? ???????? ????????
Small int:        0_______ __00 <20-bit 2's complement integer>
Symbol:           0_______ __01 <20-bit hash of symbol name>
Pair:             0_______ __10 <Index of head/tail values in heap>
Heap object:      0_______ __11 <Index of object header in heap>
Unused:           1_______ __00
Unused:           1_______ __01
Unused:           1_______ __10
Unused:           1_______ __11

When the value is a pair, the raw value is an index into the value array heap.
The head is stored at the index and the tail is stored immediately after.

When the value is a heap object, the raw value is an index into the heap where
the object header is stored. A header is a 32-bit int where the top 3 bits
encode the type and the lower 29 bits encode the header value. The header is
followed by the object's data.

Tuple:            000 <Number of items>
Big int:          001 <Undetermined>
Dict:             010 <Number of key/value entries>
Binary:           011 <Number of bytes>

A tuple is followed by N values, where N is the header value. Big ints are not
yet implemented. A dict is followed by 2 values: a tuple where the keys are
stored and a tuple where the values are stored. A binary is followed by its
bytes, padded to 32-bits. The length in terms of 4-byte words can be calculated
with `((length - 1) / 4) + 1`.

The heap allocates new objects sequentially. A special value called `nil` has
the value 0x7FE00000, which is a pair at index 0 in the heap. The heap should be
initialized to store `nil` at index 0 and 1, so that `Head(nil) == nil` and
`Tail(nil) == nil`. `nil` represents the empty list.

Along with the value array, the memory heap should also maintain a hashmap of
symbol names. A symbol's raw value is its 20-bit hash.
*/

typedef union {
  float as_f;
  i32 as_v;
} Val;

#define SignExt(n)    ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits       20

#define nanMask       0x7FC00000
#define typeMask      0xFFF00000
#define intMask       0x7FC00000
#define symMask       0x7FD00000
#define pairMask      0x7FE00000
#define objMask       0x7FF00000

#define NumVal(n)     (Val){.as_f = (float)(n)}
#define IntVal(n)     (Val){.as_v = ((n) & ~typeMask) | intMask}
#define SymVal(n)     (Val){.as_v = ((n) & ~typeMask) | symMask}
#define PairVal(n)    (Val){.as_v = ((n) & ~typeMask) | pairMask}
#define ObjVal(n)     (Val){.as_v = ((n) & ~typeMask) | objMask}

#define IsNum(v)      (((v).as_v & nanMask) != nanMask)
#define IsInt(v)      (((v).as_v & typeMask) == intMask)
#define IsSym(v)      (((v).as_v & typeMask) == symMask)
#define IsPair(v)     (((v).as_v & typeMask) == pairMask)
#define IsObj(v)      (((v).as_v & typeMask) == objMask)

#define RawNum(v)     (IsNum(v) ? (v).as_f : RawInt(v))
#define RawInt(v)     SignExt((v).as_v)
#define RawSym(v)     ((v).as_v & ~typeMask)
#define RawPair(v)    ((v).as_v & ~typeMask)
#define RawObj(v)     ((v).as_v & ~typeMask)

#define IsNumeric(n)  (IsNum(n) || IsInt(n))

#define headerMask    0xE0000000
#define tupleMask     0x00000000
#define bigIntMask    0x20000000
#define dictMask      0x40000000
#define binaryMask    0x60000000

#define TupleHeader(n)    (Val){.as_v = ((n) & ~headerMask) | tupleMask}
#define BigIntHeader(n)   (Val){.as_v = ((n) & ~headerMask) | bigIntMask}
#define DictHeader(n)     (Val){.as_v = ((n) & ~headerMask) | dictMask}
#define BigDictHeader(n)  (Val){.as_v = ((n) & ~headerMask) | bigDictMask}
#define BinaryHeader(n)   (Val){.as_v = ((n) & ~headerMask) | binaryMask}

#define HeaderVal(h)      (u32)((h).as_v & ~headerMask)
#define HeaderType(h)     (u32)((h).as_v & headerMask)

#define nil           PairVal(0)
#define IsNil(v)      (Eq(v, nil))
#define IsList(m, v)  (IsPair(v) && IsPair(Tail(m, v)))
#define BoolVal(v)    ((v) ? SymbolFor("true") : SymbolFor("false"))
#define Eq(v1, v2)    ((v1).as_v == (v2).as_v)

#define SymbolFrom(str, len)  SymVal(HashBits(str, len, valBits))
#define SymbolFor(str)        SymbolFrom(str, StrLen(str))
