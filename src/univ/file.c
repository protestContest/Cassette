#include "univ/file.h"
#include "univ/str.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

FileList *NewFileList(u32 count)
{
  FileList *list = malloc(sizeof(FileList));
  u32 i;
  list->count = count;
  list->filenames = malloc(sizeof(char*)*count);
  for (i = 0; i < count; i++) list->filenames[i] = 0;
  return list;
}

void FreeFileList(FileList *list)
{
  u32 i;
  for (i = 0; i < list->count; i++) free(list->filenames[i]);
  free(list->filenames);
  free(list);
}

i32 Open(char *path, i32 flags, char **error)
{
  i32 f = open(path, flags, 0x1FF);
  *error = (error && f < 0) ? strerror(errno) : 0;
  return f;
}

i32 OpenSerial(char *path, i32 speed, i32 opts, char **error)
{
  struct termios options;
  i32 file = open(path, O_RDWR | O_NDELAY | O_NOCTTY);
  if (file < 0) {
    *error = strerror(errno);
    return file;
  }
  *error = 0;
  fcntl(file, F_SETFL, O_NONBLOCK);
  tcgetattr(file, &options);
  cfsetispeed(&options, speed);
  cfsetospeed(&options, speed);
  options.c_cflag |= opts;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
  tcsetattr(file, TCSANOW, &options);
  return file;
}

char *Close(i32 file)
{
  i32 err = close(file);
  if (err != 0) {
    return strerror(err);
  }
  return 0;
}

i32 Read(i32 file, char *buf, u32 size, char **error)
{
  i32 num_read = read(file, buf, size);
  *error = (num_read < 0) ? strerror(errno) : 0;
  return num_read;
}

i32 Write(i32 file, char *buf, u32 size, char **error)
{
  i32 num_written = write(file, buf, size);
  *error = (num_written < 0) ? strerror(errno) : 0;
  return num_written;
}

i32 Seek(i32 file, i32 offset, i32 whence, char **error)
{
  i32 pos = lseek(file, offset, whence);
  *error = (pos < 0) ? strerror(errno) : 0;
  return pos;
}

i32 Listen(char *port, char **error)
{
  i32 status, sock;
  struct addrinfo hints = {0};
  struct addrinfo *info;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  status = getaddrinfo(0, port, &hints, &info);

  if (status != 0) {
    *error = (char*)gai_strerror(status);
    return -1;
  }

  sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sock < 0) {
    freeaddrinfo(info);
    *error = (char*)strerror(errno);
    return -1;
  }

  status = bind(sock, info->ai_addr, info->ai_addrlen);
  freeaddrinfo(info);
  if (status < 0) {
    *error = (char*)strerror(errno);
    return -1;
  }

  status = listen(sock, 10);
  if (status < 0) {
    *error = strerror(errno);
    return -1;
  }

  *error = 0;
  return sock;
}

i32 Accept(i32 sock, char **error)
{
  i32 s = accept(sock, 0, 0);
  *error = (s < 0) ? strerror(errno) : 0;
  return s;
}

i32 Connect(char *node, char *port, char **error)
{
  struct addrinfo hints = {0};
  struct addrinfo *servinfo;
  i32 status, s;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(node, port, &hints, &servinfo);
  if (status != 0) {
    *error = (char*)gai_strerror(status);
    return -1;
  }

  s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s < 0) {
    freeaddrinfo(servinfo);
    *error = strerror(errno);
    return s;
  }

  status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);

  *error = (status < 0) ? *error = strerror(errno) : 0;
  return s;
}

char *ReadFile(char *path)
{
  int file;
  u32 size;
  char *data;

  file = open(path, O_RDWR, 0);
  if (file < 0) return 0;
  size = lseek(file, 0, 2);
  lseek(file, 0, 0);
  data = malloc(size + 1);
  read(file, data, size);
  data[size] = 0;
  close(file);
  return data;
}

i32 WriteFile(u8 *data, u32 size, char *path)
{
  int file;
  i32 written;
  u32 mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  file = open(path, O_RDWR | O_CREAT | O_TRUNC, mode);
  if (file < 0) return -1;
  written = write(file, data, size);
  close(file);
  return written;
}

static int OnlyFiles(const struct dirent *entry)
{
  return entry->d_type == DT_REG && entry->d_name[0] != '.';
}

static int OnlyFolders(const struct dirent *entry)
{
  return entry->d_type == DT_DIR && entry->d_name[0] != '.';
}

FileList *ListFiles(char *path, char *ext, FileList *list)
{
  struct dirent **filenames;
  struct dirent **foldernames;
  i32 num_files, num_folders, num_filtered, i;

  if (!list) {
    list = malloc(sizeof(FileList));
    list->count = 0;
    list->filenames = 0;
  }

  num_files = scandir(path, &filenames, OnlyFiles, 0);
  num_folders = scandir(path, &foldernames, OnlyFolders, 0);

  if (num_files <= 0 && num_folders <= 0) return list;

  num_filtered = num_files;
  if (ext) {
    for (i = 0; i < num_files; i++) {
      char *fext = FileExt(filenames[i]->d_name);
      if (!StrEq(ext, fext)) {
        num_filtered--;
        free(filenames[i]);
        filenames[i] = 0;
      }
    }
  }

  list->filenames = realloc(list->filenames,
                            sizeof(char*)*(list->count + num_filtered));

  for (i = 0; i < num_files; i++) {
    if (filenames[i]) {
      list->filenames[list->count++] = JoinStr(path, filenames[i]->d_name, '/');
      free(filenames[i]);
    }
  }
  free(filenames);

  for (i = 0; i < num_folders; i++) {
    char *foldername = JoinStr(path, foldernames[i]->d_name, '/');
    list = ListFiles(foldername, ext, list);
    free(foldername);
    free(foldernames[i]);
  }
  free(foldernames);

  return list;
}

char *FileExt(char *path)
{
  return LastIndex(path, '.');
}

char *DirName(char *path)
{
  return dirname(path);
}

bool DirExists(char *path)
{
  struct stat info;
  return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

char *HomeDir(void)
{
  struct passwd *pw = getpwuid(getuid());
  return pw->pw_dir;
}
