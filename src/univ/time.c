#include "univ/time.h"
#include <sys/time.h>

u32 Time(void)
{
  struct timeval ts;
  gettimeofday(&ts, NULL);
  return ts.tv_sec;
}

u64 Microtime(void)
{
  struct timeval ts;
  gettimeofday(&ts, NULL);
  return 1000000*ts.tv_sec + ts.tv_usec;
}
