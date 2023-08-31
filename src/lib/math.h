#pragma once
#include "vm.h"

Val MathRandom(VM *vm, Val args);
Val MathRandInt(VM *vm, Val args);
Val MathCeil(VM *vm, Val args);
Val MathFloor(VM *vm, Val args);
Val MathRound(VM *vm, Val args);
Val MathAbs(VM *vm, Val args);
Val MathMax(VM *vm, Val args);
Val MathMin(VM *vm, Val args);
Val MathSin(VM *vm, Val args);
Val MathCos(VM *vm, Val args);
Val MathTan(VM *vm, Val args);
Val MathLn(VM *vm, Val args);
Val MathExp(VM *vm, Val args);
Val MathPow(VM *vm, Val args);
Val MathSqrt(VM *vm, Val args);
Val MathPi(VM *vm, Val args);
Val MathE(VM *vm, Val args);
Val MathInfinity(VM *vm, Val args);
