#pragma once
#include "chunk.h"

char *ReadFile(const char *path);
Status LoadFile(char *path, Chunk *chunk);
Status LoadModules(char *dir_name, char *main_file, Chunk *chunk);
