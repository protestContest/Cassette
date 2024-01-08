#include "result.h"
#include "univ/str.h"
#include "univ/system.h"

Result ValueResult(u32 value)
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
  error->value = 0;
  error->item = 0;

  result.ok = false;
  result.data.error = error;
  return result;
}
