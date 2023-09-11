#include "system.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void Exit(void)
{
  exit(0);
}

i32 Open(char *path)
{
  mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
  return open(path, O_RDWR|O_CREAT, mode);
}

bool Read(i32 file, void *data, u32 length)
{
  return read(file, data, length) >= 0;
}

bool Write(i32 file, void *data, u32 length)
{
  return write(file, data, length) >= 0;
}
