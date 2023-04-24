#pragma once
#include "vm.h"

Val OpenPort(VM *vm, Val type, Val name, Val init);
Val SendPort(VM *vm, Val port, Val message);
Val RecvPort(VM *vm, Val port);

bool HasPort(VM *vm, Val type, Val name);
Val GetPort(VM *vm, Val type, Val name);
