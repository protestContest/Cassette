/*
A result object indicates success or failure of some operation, plus the success
value or error details.
*/

#include "result.h"
#include "univ/system.h"
#include "univ/str.h"
#include "mem/symbols.h"

Result ValueResult(Val value)
{
  Result result;
  result.ok = true;
  result.data.value = value;
  return result;
}

Result ItemResult(void *item)
{
  Result result;
  result.ok = true;
  result.data.item = item;
  return result;
}

Result ErrorResult(char *message, char *filename, u32 pos)
{
  Result result;
  ErrorDetails *error = Alloc(sizeof(ErrorDetails));
  error->message = message;
  if (filename) {
    error->filename = CopyStr(filename, StrLen(filename));
    error->pos = pos;
  } else {
    error->filename = 0;
    error->pos = 0;
  }
  error->value = Nil;
  error->item = 0;

  result.ok = false;
  result.data.error = error;
  return result;
}

Result OkError(Result result, Mem *mem)
{
  char *message = ResultError(result)->message;
  return ValueResult(Pair(Error, BinaryFrom(message, StrLen(message), mem), mem));
}
