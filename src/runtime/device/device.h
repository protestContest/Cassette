#pragma once
#include "runtime/mem/mem.h"
#include "univ/result.h"

typedef enum {
  ConsoleDevice,
  FileDevice,
  DirectoryDevice,
  SerialDevice,
  SystemDevice,
  WindowDevice
} DeviceType;

typedef struct {
  DeviceType type;
  void *context;
} Device;

typedef Result (*DeviceOpenFn)(Val opts, Mem *mem);
typedef Result (*DeviceCloseFn)(void *context, Mem *mem);
typedef Result (*DeviceReadFn)(void *context, Val length, Mem *mem);
typedef Result (*DeviceWriteFn)(void *context, Val data, Mem *mem);
typedef Result (*DeviceSetFn)(void *context, Val key, Val value, Mem *mem);
typedef Result (*DeviceGetFn)(void *context, Val key, Mem *mem);

typedef struct {
  DeviceType type;
  Val name;
  DeviceOpenFn open;
  DeviceCloseFn close;
  DeviceReadFn read;
  DeviceWriteFn write;
  DeviceSetFn set;
  DeviceGetFn get;
} DeviceDriver;

DeviceType GetDeviceType(Val symbol);
u32 GetDevices(DeviceDriver **devices);

Result DeviceOpen(DeviceType type, Val opts, Mem *mem);
Result DeviceClose(Device *device, Mem *mem);
Result DeviceRead(Device *device, Val length, Mem *mem);
Result DeviceWrite(Device *device, Val data, Mem *mem);
Result DeviceSet(Device *device, Val key, Val value, Mem *mem);
Result DeviceGet(Device *device, Val key, Mem *mem);
