#pragma once
#include "vm.h"

void REPL(VM *vm);
char *ReadFile(const char *path);
void RunFile(VM *vm, const char *path);
