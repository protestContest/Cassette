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
  return result;
}

Result ErrorResult(char *error, char *filename, u32 pos)
{
  /* It's important to copy the filename for the result, since the original
  owner will probably be freed by the time we want to print it */
  Result result;
  u32 filename_len = StrLen(filename);
  result.ok = false;
  result.value = Error;
  result.error = error;
  result.filename = Alloc(filename_len+1);
  result.pos = pos;
  Copy(filename, result.filename, filename_len+1);
  return result;
}
