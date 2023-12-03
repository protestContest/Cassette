/*
A result object indicates success or failure of some operation, plus the success
value or error details.
*/

#include "result.h"
#include "univ/system.h"
#include "univ/str.h"
#include "mem/symbols.h"

Result OkResult(Val value)
{
  Result result;
  result.ok = true;
  result.value = value;
  result.error = 0;
  result.filename = 0;
  result.pos = 0;
  result.data = 0;
  return result;
}

Result DataResult(void *data)
{
  Result result = OkResult(Nil);
  result.data = data;
  return result;
}

Result ErrorResult(char *error, char *filename, u32 pos)
{
  /* It's important to copy the filename for the result, since the original
  owner will probably be freed by the time we want to print it */
  Result result;
  result.ok = false;
  result.value = Error;
  result.error = error;
  result.data = 0;
  if (filename) {
    u32 filename_len = StrLen(filename);
    result.filename = Alloc(filename_len+1);
    result.pos = pos;
    Copy(filename, result.filename, filename_len+1);
  } else {
    result.filename = 0;
    result.pos = 0;
  }
  return result;
}

Result OkError(Result error, Mem *mem)
{
  return OkResult(Pair(Error, BinaryFrom(error.error, StrLen(error.error), mem), mem));
}
