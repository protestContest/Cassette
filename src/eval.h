#pragma once
#include "value.h"

#define DEBUG_EVAL 0

typedef struct {
  enum { EVAL_OK, EVAL_ERROR } status;
  union {
    Val value;
    char *error;
  };
} EvalResult;

EvalResult Eval(Val exp, Val env);
EvalResult Apply(Val proc, Val args);
EvalResult EvalDefine(Val exp, Val env);
EvalResult EvalModule(Val exp, Val env);
EvalResult EvalModuleBody(Val body, Val env);

bool IsSelfEvaluating(Val exp);
bool IsAccessable(Val exp);

EvalResult EvalOk(Val exp);
EvalResult RuntimeError(char *msg);

void PrintEvalError(EvalResult result);
