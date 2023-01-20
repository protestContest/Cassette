#pragma once
#include "value.h"

#define DEBUG_EVAL 0

typedef struct {
  enum { EVAL_OK, EVAL_ERROR } status;
  Val value;
  char *error;
} EvalResult;

EvalResult Eval(Val exp, Val env);
EvalResult Apply(Val proc, Val args);

EvalResult EvalOk(Val exp);
EvalResult RuntimeError(char *msg);

void PrintEvalError(EvalResult result);
