#pragma once
#include "vec.h"

typedef struct {
  char *name;
  char *path;
  i32 file;
} SerialPort;

ObjVec *ListSerialPorts(void);

i32 OpenSerial(char *path, u32 speed);
void CloseSerial(int file);
