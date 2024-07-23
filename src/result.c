#include "result.h"
#include <univ/str.h>

Result Ok(void *data)
{
  Result result;
  result.status = ok;
  result.data.p = data;
  return result;
}

Result OkVal(u32 value)
{
  Result result;
  result.status = ok;
  result.data.v = value;
  return result;
}

Result Error(StatusCode errorCode, char *message)
{
  Result result;
  result.status = errorCode;
  result.data.p = message;
  return result;
}

char *StatusMessage(StatusCode code)
{
  switch (code) {
  case ok: return "ok";
  case parseError: return "parseError";
  case compileError: return "compileError";
  case moduleNotFound: return "moduleNotFound";
  case duplicateModule: return "duplicateModule";
  case undefinedVariable: return "undefinedVariable";
  case stackUnderflow: return "stackUnderflow";
  case invalidType: return "invalidType";
  case divideByZero: return "divideByZero";
  case outOfBounds: return "outOfBounds";
  case unhandledTrap: return "unhandledTrap";
  }
}

void PrintError(Result error)
{
  fprintf(stderr, "%s%s%s\n", ANSIRed, StatusMessage(error.status), ANSINormal);
}
