#include "io.h"

#ifdef _POSIX_VERSION
bool OSWrite(int file, void *data, u32 length)
{
  return write(file, data, length) >= 0;
}
#else
bool OSWrite(int file, void *data, u32 length)
{
  return false;
}
#endif
