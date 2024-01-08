#include "serial.h"
#include "str.h"
#include "system.h"
#include <fcntl.h>
#include <termios.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if PLATFORM == Apple

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>

#define GetIOProperty(io, name) IORegistryEntrySearchCFProperty(io, kIOServicePlane, CFSTR(name), kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents)
#define CreateIOProperty(io, key) IORegistryEntryCreateCFProperty(io, CFSTR(key), kCFAllocatorDefault, 0)

static char *RefName(CFTypeRef ref)
{
  char tmp[PATH_MAX];
  char *str = 0;

  if (!ref) return 0;
  if (CFStringGetCString(ref, tmp, sizeof(tmp), kCFStringEncodingASCII)) {
    str = CopyStr(tmp, StrLen(tmp));
  }
  CFRelease(ref);
  return str;
}

static char *GetPortPath(io_object_t port)
{
  return RefName(CreateIOProperty(port, kIOCalloutDeviceKey));
}

static char *GetPortName(io_object_t ioport)
{
  CFTypeRef ref;
  ref = GetIOProperty(ioport, "USB Serial Number");
  ref = (ref) ? ref : GetIOProperty(ioport, "USB Interface Name");
  ref = (ref) ? ref : GetIOProperty(ioport, "USB Interface Name");
  ref = (ref) ? ref : GetIOProperty(ioport, "USB Product Name");
  ref = (ref) ? ref : GetIOProperty(ioport, "Product Name");
  ref = (ref) ? ref : IORegistryEntryCreateCFProperty(ioport, CFSTR(kIOTTYDeviceKey), kCFAllocatorDefault, 0);
  return RefName(ref);
}

ObjVec *ListSerialPorts(void)
{
  CFMutableDictionaryRef dict;
  io_iterator_t iter;
  io_object_t port;
  ObjVec *list = Alloc(sizeof(ObjVec));

  InitVec(list, sizeof(void*), 8);

  dict = IOServiceMatching(kIOSerialBSDServiceValue);
  if (!dict) return list;

  if (IOServiceGetMatchingServices(kIOMainPortDefault, dict, &iter) != KERN_SUCCESS) return list;

  port = IOIteratorNext(iter);
  while (port) {
    char *path = GetPortPath(port);
    if (path) {
      SerialPort *serial = Alloc(sizeof(SerialPort));
      serial->path = path;
      serial->name = GetPortName(port);
      serial->file = -1;
      ObjVecPush(list, serial);
    }

    IOObjectRelease(port);
    port = IOIteratorNext(iter);
  }

  return list;
}

#else

ObjVec *ListSerialPorts(void)
{
  ObjVec *list = Alloc(sizeof(ObjVec));
  InitVec(list, sizeof(void*), 0);
  return list;
}

#endif

i32 OpenSerial(char *path, u32 speed)
{
  int file;
  struct termios  options;

  file = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (file == -1) return -1;

  /* make file access exclusive */
  if (ioctl(file, TIOCEXCL) == -1) {
    close(file);
    return -1;
  }

  /* set to blocking IO */
  if (fcntl(file, F_SETFL, 0) == -1) {
    close(file);
    return -1;
  }

  /* get the current attributes */
  if (tcgetattr(file, &options) == -1) {
    close(file);
    return -1;
  }

  cfmakeraw(&options);
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 10;

  /* set speed & options */
  cfsetspeed(&options, speed);
  options.c_cflag |= (CS8 | PARENB);

  /* make options take effect */
  if (tcsetattr(file, TCSANOW, &options) == -1) {
    close(file);
    return -1;
  }

  return file;
}

void CloseSerial(int file)
{
  /* block until all data is sent */
  tcdrain(file);
  close(file);
}
