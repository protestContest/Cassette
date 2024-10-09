#include "result.h"

Result Ok(void *data)
{
  Result result;
  result.ok = true;
  result.data.p = data;
  return result;
}

Result OkVal(u32 value)
{
  Result result;
  result.ok = true;
  result.data.v = value;
  return result;
}

Result Err(Error *error)
{
  Result result;
  result.ok = false;
  result.data.p = error;
  return result;
}
