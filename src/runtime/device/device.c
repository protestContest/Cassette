#include "device.h"
#include "console.h"
#include "directory.h"
#include "file.h"
#include "serial.h"
#include "system.h"
#include "window.h"

#define SymConsole        0x7FD8CD69  /* console */
#define SymFile           0x7FDB7595  /* file */
#define SymDirectory      0x7FD7808F  /* directory */
#define SymSerial         0x7FD0DEF4  /* serial */
#define SymSystem         0x7FDDFAA0  /* system */
#define SymWindow         0x7FDDF3AE  /* window */

static DeviceDriver drivers[] = {
  {
    ConsoleDevice,
    SymConsole,
    0, /* open */
    0, /* close */
    ConsoleRead,
    ConsoleWrite,
    0, /* set */
    0  /* get */
  },
  {
    FileDevice,
    SymFile,
    FileOpen,
    FileClose,
    FileRead,
    FileWrite,
    FileSet,
    FileGet},
  {
    DirectoryDevice,
    SymDirectory,
    DirectoryOpen,
    DirectoryClose,
    DirectoryRead,
    DirectoryWrite,
    0, /* set */
    DirectoryGet
  },
  {
    SerialDevice,
    SymSerial,
    SerialOpen,
    SerialClose,
    SerialRead,
    SerialWrite,
    SerialSet,
    SerialGet
  },
  {
    SystemDevice,
    SymSystem,
    0, /* open */
    0, /* close */
    0, /* read */
    0, /* write */
    SystemSet,
    SystemGet
  },
  {
    WindowDevice,
    SymWindow,
    WindowOpen,
    WindowClose,
    WindowRead,
    WindowWrite,
    WindowSet,
    WindowGet
  }
};


DeviceType GetDeviceType(Val symbol)
{
  switch (symbol) {
  case SymConsole:    return ConsoleDevice;
  case SymFile:       return FileDevice;
  case SymDirectory:  return DirectoryDevice;
  case SymSerial:     return SerialDevice;
  case SymSystem:     return SystemDevice;
  case SymWindow:     return WindowDevice;
  default: return -1;
  }
}

u32 GetDevices(DeviceDriver **devices)
{
  *devices = drivers;
  return ArrayCount(drivers);
}

Result DeviceOpen(DeviceType type, Val opts, Mem *mem)
{
  if (drivers[type].open) {
    return drivers[type].open(opts, mem);
  } else {
    return ValueResult(Nil);
  }
}

Result DeviceClose(Device *device, Mem *mem)
{
  if (drivers[device->type].close) {
    return drivers[device->type].close(device->context, mem);
  } else {
    return ValueResult(Nil);
  }
}

Result DeviceRead(Device *device, Val length, Mem *mem)
{
  if (drivers[device->type].read) {
    return drivers[device->type].read(device->context, length, mem);
  } else {
    return ValueResult(Nil);
  }
}

Result DeviceWrite(Device *device, Val data, Mem *mem)
{
  if (drivers[device->type].write) {
    return drivers[device->type].write(device->context, data, mem);
  } else {
    return ValueResult(Nil);
  }
}

Result DeviceSet(Device *device, Val key, Val value, Mem *mem)
{
  if (drivers[device->type].set) {
    return drivers[device->type].set(device->context, key, value, mem);
  } else {
    return ValueResult(Nil);
  }
}

Result DeviceGet(Device *device, Val key, Mem *mem)
{
  if (drivers[device->type].get) {
    return drivers[device->type].get(device->context, key, mem);
  } else {
    return ValueResult(Nil);
  }
}


