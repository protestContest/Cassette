#pragma once

#include "result.h"
#include "runtime/chunk.h"

typedef struct {
  char *name;
  ObjVec files;
} Manifest;

void InitManifest(Manifest *manifest);
void DestroyManifest(Manifest *manifest);
bool ReadManifest(char *filename, Manifest *manifest);
Result BuildProject(Manifest *manifest, Chunk *chunk);
