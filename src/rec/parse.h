#pragma once
#include "heap.h"
#include "lex.h"
#include "source.h"

#define DoTag       (Val){.as_i = 0x7FDC98A0}
#define ImportTag   (Val){.as_i = 0x7FDD35FE}
#define LetTag      (Val){.as_i = 0x7FD90E31}
#define LambdaTag   (Val){.as_i = 0x7FD0BFAA}
#define QuoteTag    (Val){.as_i = 0x7FDC4FCD}
#define SymbolTag   (Val){.as_i = 0x7FDC3CE9}
#define ListTag     (Val){.as_i = 0x7FDC56E3}
#define IfTag       (Val){.as_i = 0x7FDAED39}
#define NilTag      (Val){.as_i = 0x7FD02CCD}
#define TupleTag    (Val){.as_i = 0x7FDC7013}
#define MapTag      (Val){.as_i = 0x7FD23432}
#define AccessTag   (Val){.as_i = 0x7FDC465B}
#define ModuleTag   (Val){.as_i = 0x7FDBEA74}

typedef struct {
  bool ok;
  union {
    Val value;
    CompileError error;
  };
} ParseResult;

ParseResult Parse(char *source, Heap *mem);
