#include "univ/time.h"
#include <sys/time.h>
#include <time.h>

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

u64 Ticks(void)
{
  struct timespec res, tp;
  clock_getres(CLOCK_THREAD_CPUTIME_ID, &res);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tp);
  return (tp.tv_sec * 1000*1000*1000 + tp.tv_nsec) / res.tv_nsec;
}
