#include "file.h"

// #define DEBUG_MODULE 1

char *ReadFile(const char *path)
{
  FILE *file = fopen(path, "rb");
  if (!file) Fatal("Could not open \"%s\"", path);

  fseek(file, 0L, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char *buffer = (char*)malloc(size + 1);
  size_t num_read = fread(buffer, sizeof(char), size, file);
  buffer[num_read] = '\0';
  fclose(file);
  return buffer;
}

// Status LoadFile(char *path, Chunk *chunk)
// {
//   if (DEBUG_MODULE) printf("Loading \"%s\"\n", path);

//   char *src = ReadFile(path);
//   Status result = CompileModule(src, chunk);
//   free(src);
//   return result;
// }

// Status LoadModules(char *dir_name, char *main_file, Chunk *chunk)
// {
//   DIR *dir = opendir(dir_name);
//   if (dir == NULL) {
//     printf("Bad directory name: %s\n", dir_name);
//     return Error;
//   }

//   struct dirent *entry;
//   while ((entry = readdir(dir)) != NULL) {
//     if (entry->d_type == DT_DIR) {
//       char path[1024];
//       if (entry->d_name[0] == '.' && (!main_file || strcmp(entry->d_name, main_file) != 0)) continue;
//       snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
//       Status status = LoadModules(path, "", chunk);
//       if (status != Ok) return status;
//     } else if (entry->d_type == DT_REG
//         && entry->d_namlen > 4
//         && (!main_file || strcmp(entry->d_name, main_file) != 0)) {
//       if (strcmp(entry->d_name + (entry->d_namlen - 4), ".rye") == 0) {
//         char path[1024];
//         snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
//         Status status = LoadFile(entry->d_name, chunk);
//         if (status != Ok) return status;
//       }
//     }
//   }

//   return Ok;
// }
