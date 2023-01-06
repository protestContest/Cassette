#pragma once
#include "value.h"

typedef enum {
  OP_NOP,
  OP_CNST,  // <n> <dst>    ; move const number n to reg dst ; move n(a0), d0
  OP_MOVE,  // <src> <dst>  ; move reg src to reg dst        ; move d0, d1
  OP_LEA,   // <n> <dst>    ; moves the address a into the reg dst ; lea ea,a1
  OP_CMP,   // <a> <b>      ; sets flags according to (reg b) - (reg a) ; cmp d0 d1
  OP_CMPI,  // <n> <b>      ; sets flags according to (reg b) - n ; cmpi #8 d1
  OP_BRA,   // <offset>     ; branch to pc + offset          ; bra 8
  OP_BEQ,   // <offset>     ; branch if Z flag is set        ; beq 8
  OP_BNE,   // <offset>     ; branch if Z flag is clear      ; bne 8
  OP_BLT,   // <offset>     ; branch if V and N flags differ ; blt 8
  OP_BLE,   // <offset>     ; branch if Z flag is set or V and N flags differ
  OP_BGT,   // <offset>     ; branch if Z flag is clear and V and N flags are the same
  OP_BGE,   // <offset>     ; branch if V and N flags are the same
  OP_SAVE,  // <reg>        ; push reg onto the stack (increments sp) ; move d0 -(sp)
  OP_RSTR,  // <reg>        ; pop stack into reg (decrements sp)      ; move (sp)+ d0
  OP_BRK,   //              ; pauses execution
  OP_STOP,  //              ; halt
  OP_SUB,   // <a> <b>      ; reg b - reg a   ; sub d0 d1
  OP_EXG,
} OpCode;

typedef struct {
  const char *name;
  u32 time;
  u32 size;
} OpStat;

typedef struct {
  u32 count;
  u32 capacity;
  u8 *code;
  struct {
    u32 count;
    u32 capacity;
    Val *values;
  } constants;
} Chunk;

enum { D0, D1, D2, D3, D4, D5, D6, D7 };
enum { A0, A1, A2, A3, A4, A5, A6, A7 };
#define SP  a[7]

typedef struct {
  u32 clock;
  bool carry:1;
  bool overflow:1;
  bool zero:1;
  bool negative:1;
  bool extend:1;
  u32 pc;
  Val d[8];
  u32 a[8];
  Val *stack;
  Chunk *chunk;
} VM;

Chunk *NewChunk(void);
void PutChunk(Chunk *chunk, u8 byte);
u8 PutConstant(Chunk *chunk, Val constant);
Val ChunkConstant(Chunk *chunk, u8 index);
u32 PutInst(Chunk *chunk, OpCode op, ...);

void Disassemble(Chunk *chunk, char *name);

VM *NewVM(void);
void ExecuteChunk(VM *vm, Chunk *chunk);
void DumpVM(VM *vm);
