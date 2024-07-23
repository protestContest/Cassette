#pragma once

/* These are the VM's op codes */

typedef enum {
  opNoop,   /* */
  opConst,  /* a; -> a */
  opAdd,    /* b a -> (a+b) */
  opSub,    /* b a -> (a-b) */
  opMul,    /* b a -> (a*b) */
  opDiv,    /* b a -> (a/b) */
  opRem,    /* b a -> (a%b) */
  opAnd,    /* b a -> (a&b) */
  opOr,     /* b a -> (a|b) */
  opComp,   /* a -> (~a) */
  opLt,     /* b a -> (a<b) */
  opGt,     /* b a -> (a>b) */
  opEq,     /* b a -> (a==b) */
  opNeg,    /* a -> (-a) */
  opNot,    /* a -> (!a) */
  opShift,  /* b a -> (a<<b) */
  opPair,   /* b a -> pair(b,a) */
  opHead,   /* a -> head(a) */
  opTail,   /* a -> tail(a) */
  opTuple,  /* n; -> tuple(n) */
  opLen,    /* a -> len(a) */
  opGet,    /* b a -> (a[b]) */
  opSet,    /* c b a -> a; a[b] <- c */
  opStr,    /* a -> sym_to_str(a) */
  opJoin,   /* b a -> (a <> b) */
  opSlice,  /* c b a -> (a[b:c]) */
  opJump,   /* n; pc <- pc + n */
  opBranch, /* n; a -> ; pc <- pc + n */
  opTrap,   /* n; ?? -> a */
  opPos,    /* n; -> (pc + n) */
  opGoto,   /* a -> ; pc <- pc + a */
  opHalt,   /* */
  opDup,    /* a -> a a */
  opDrop,   /* a -> */
  opSwap,   /* b a -> a b */
  opOver,   /* b a -> a b a */
  opRot,    /* c b a -> a c b */
  opGetEnv, /* -> env */
  opSetEnv, /* a -> ; env <- a */
  opGetMod, /* -> mods */
  opSetMod  /* a -> ; mods <- a */
} OpCode;

char *OpName(OpCode op);
u32 DisassembleInst(u8 *code, u32 *index);
void Disassemble(u8 *code);
