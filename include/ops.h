#pragma once

/*
  These are the VM's op codes.
  Each op code has a stack diagram. The right most value is on top.
  Some ops take an inline argument, denoted by being in parentheses.
*/

typedef enum {
  opNoop = 0x00,   /* */
  opHalt,   /* */
  opConst,  /* (a) -> a           pushes a tagged value */

  opJump = 0x10,   /* (n) pc <- pc + n */
  opBranch, /* (n) a -> ; pc <- pc + n */
  opPos,    /* (n) -> pc + n */
  opGoto,   /* a -> ; pc <- pc + a */
  opGetEnv, /* -> env */
  opSetEnv, /* a -> ; env <- a */
  opGetMod, /* -> mods */
  opSetMod, /* a -> ; mods <- a */
  opLink,   /* -> link; link <- #stack */
  opUnlink, /* a -> ; link <- a */

  opAdd = 0x20,    /* a b -> a+b */
  opSub,    /* a b -> a-b */
  opMul,    /* a b -> a*b */
  opDiv,    /* a b -> a/b */
  opRem,    /* a b -> a%b */
  opAnd,    /* a b -> a&b */
  opOr,     /* a b -> a|b */
  opComp,   /* a -> ~a */
  opLt,     /* a b -> a<b */
  opGt,     /* a b -> a>b */
  opEq,     /* a b -> a==b */
  opNeg,    /* a -> -a */
  opNot,    /* a -> !a */
  opShift,  /* a b -> a<<b */

  opDup = 0x30,    /* a -> a a */
  opDrop,   /* a -> */
  opSwap,   /* a b -> b a */
  opOver,   /* a b -> a b a */
  opRot,    /* a b c -> b c a */
  opPick,   /* n; a ... z -> a ... z a */
  opRoll,   /* n; a b ... z -> a ... z b */

  opPair = 0x40,   /* t h -> pair(h,t)   creates a pair (may GC) */
  opHead,   /* p -> head(p) */
  opTail,   /* p -> tail(p) */
  opTuple,  /* (n) -> tuple(n)    creates a tuple of length n (may GC) */
  opLen,    /* a -> len(a) */
  opGet,    /* a b -> a[b] */
  opSet,    /* a b c -> a         sets a[b] := c */
  opStr,    /* a -> str(a)        must be a symbol (may GC) */
  opJoin,   /* a b -> a<>b        (may GC) */
  opSlice,  /* a b c -> a[b:c]    (may GC) */

  opTrap = 0x7F    /* (n) ?? -> a        calls a native trap function (may GC) */
} OpCode;

char *OpName(OpCode op);
u32 DisassembleInst(u8 *code, u32 *index);
void Disassemble(u8 *code);
