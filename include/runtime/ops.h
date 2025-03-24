#pragma once
#include "vm.h"

/*
 * These are the VM's op codes.
 * Each op code has a stack diagram.
 * Some ops take an inline LEB-encoded argument.
 */

typedef enum {
  opNoop = 0x00,
  opHalt,
  opPanic,
  opConst,        /* const a;   _ -> a
                  Pushes the constant a onto the stack */
  opLookup,       /* lookup n;  env -> a
                  Given an environment on the stack, pushes the value at
                  lexical address n */
  opDefine,       /* define n;  env a -> _
                  Sets the value a at lexical address n in an environment */

  opJump = 0x10,  /* jump n
                  Adds n to pc */
  opBranch,       /* branch n;  a -> _
                  Adds n to pc if a is not nil or zero */
  opPos,          /* pos n;     _ -> m
                  Pushes the value pc + n */
  opGoto,         /* goto;      a -> _
                  Jump to position a */
  opPush,         /* push n;    _ -> a
                  Pushes value in register n */
  opPull,         /* pull n;    a -> _
                  Sets register n to a */
  opLink,         /* link;      _ -> l
                  Pushes the link register, then sets link to the
                  current stack size */
  opUnlink,       /* unlink;    l -> _
                  Sets the link register to l */

  opAdd = 0x20,   /* add;       a b -> (a+b) */
  opSub,          /* sub;       a b -> (a-b) */
  opMul,          /* mul;       a b -> (a*b) */
  opDiv,          /* div;       a b -> (a/b) */
  opRem,          /* rem;       a b -> (a%b) */
  opAnd,          /* and;       a b -> (a&b) */
  opOr,           /* or;        a b -> (a|b) */
  opComp,         /* comp;      a -> (~a) */
  opLt,           /* lt;        a b -> (a<b) */
  opGt,           /* gt;        a b -> (a>b) */
  opEq,           /* eq;        a b -> (a==b) */
  opNeg,          /* neg;       a -> (-a) */
  opNot,          /* not;       a -> (!a) */
  opShift,        /* shift;     a b -> (a<<b) */
  opXor,          /* xor;       a b -> (a^b) */

  opDup = 0x30,   /* dup;       a -> a a */
  opDrop,         /* drop;      a -> _ */
  opSwap,         /* swap;      a b -> b a */
  opOver,         /* over;      a b -> a b a */
  opRot,          /* rot;       a b c -> b c a */
  opPick,         /* pick n;    a ... z -> a ... z a
                  Pushes the value n places from the top of the stack */

  opPair = 0x40,  /* pair;      t h -> p
                  Creates a pair */
  opHead,         /* head;      p -> h
                  Fetches the head of a pair */
  opTail,         /* tail;      p -> t
                  Fetches the tail of a pair */
  opTuple,        /* tuple n;   _ -> t
                  Creates a tuple of length n */
  opLen,          /* len;       a -> n
                  Pushes the length of a tuple or binary */
  opGet,          /* get;       a b -> a[b]
                  Fetches element b of tuple or binary a */
  opSet,          /* set;       a b c -> a
                  Sets element b of tuple or binary a to c */
  opStr,          /* str;       a -> b
                  Creates a binary from the name of symbol a */
  opJoin,         /* join;      a b -> c
                  Concatenates tuples or binaries a and b */
  opSlice,        /* slice;     a b c -> a[b:c]
                  Creates a slice of tuple or binary a from b to c */

  opTrap = 0x7F   /* trap n;    ... -> a
                  Invokes a primitive function */
} OpCode;

char *OpName(OpCode op);
u32 DisassembleInst(u8 *code /* vec */, u32 *index);
void Disassemble(u8 *code /* vec */);

void ExecOp(OpCode op, VM *vm);
