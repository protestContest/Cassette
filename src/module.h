#pragma once
#include "value.h"
#include "eval.h"

#define DEBUG_MODULE 1

typedef struct Module {
  char *path;
  Val defs;
  struct Module *next;
} Module;

Module *LoadFile(char *path);
EvalResult RunFile(char *path, Val env);
Module *LoadModules(char *name, char *main);
Val RunProject(char *main_file);
