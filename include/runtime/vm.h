#pragma once
#include "compile/opts.h"
#include "runtime/error.h"
#include "runtime/program.h"

/*
A VM can execute bytecode in a Program. Each instruction is executed until the
end is reached, or until a runtime error occurs.

The VM has 8 "registers" that can be freely manipulated with the push and pull
instructions. The compiler uses r0 to store the environment and r1 to store a
tuple of module exports.

The VM has a program counter, pc, which tracks the current execution position.
It also has a separate "link" register, which is used with the link and unlink
instructions to keep track of the call stack.

The VM has a list of "refs", which are opaque pointers that primitive functions
can create and use. For example, a primitive function can create a complex
structure, add it as a reference to the VM, and return the reference integer to
the program code.
*/

struct VM;

enum {regEnv, regMod};

typedef struct VM {
  Error *error; /* borrowed */
  u32 regs[8];
  u32 pc;
  u32 link;
  Program *program; /* borrowed */
  void **refs; /* vec, each borrowed */
  Opts *opts;
} VM;

void InitVM(VM *vm, Program *program, Opts *opts); /* prepare the VM to run a program */
void DestroyVM(VM *vm);
void VMStep(VM *vm); /* execute one instruction */
Error *VMRun(Program *program, Opts *opts); /* execute an entire program */
u32 VMPushRef(void *ref, VM *vm); /* create a ref */
void *VMGetRef(u32 ref, VM *vm); /* get the value of a ref */
i32 VMFindRef(void *ref, VM *vm); /* find a ref */
u32 RuntimeError(char *message, VM *vm); /* create a runtime error */
