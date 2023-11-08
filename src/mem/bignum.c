#include "bignum.h"
#include "univ/system.h"
#include "univ/math.h"
#include <stdio.h>

Val MakeBignum(i32 num, Mem *mem)
{
  Val bignum = ObjVal(mem->count);
  if (num < 0) {
    PushMem(mem, BignumHeader(-1));
  } else {
    PushMem(mem, BignumHeader(1));
  }
  PushMem(mem, Abs(num));
  return bignum;
}

bool IsBignum(Val num, Mem *mem)
{
  return IsObj(num) && IsBignumHeader(mem->values[RawVal(num)]);
}

bool IsBignumNegative(Val num, Mem *mem)
{
  return RawInt(mem->values[RawVal(num)]) < 0;
}

u32 BignumDigits(Val num, Mem *mem)
{
  i32 header = RawInt(mem->values[RawVal(num)]);
  return Abs(header);
}

Val AddBignum(Val a, Val b, Mem *mem)
{
  u32 a_digits, b_digits, i, overflow;
  bool a_neg, b_neg;
  Val result;
  Assert(IsBignum(a, mem) && IsBignum(b, mem));

  a_digits = BignumDigits(a, mem);
  b_digits = BignumDigits(b, mem);

  if (b_digits > a_digits) return AddBignum(b, a, mem);
  a_neg = IsBignumNegative(a, mem);
  b_neg = IsBignumNegative(b, mem);

  result = ObjVal(mem->count);
  PushMem(mem, mem->values[RawVal(a)]);
  for (i = 0; i < a_digits; i++) PushMem(mem, 0);

  overflow = 0;
  for (i = 0; i < a_digits; i++) {
    u32 a_digit = mem->values[RawVal(a)+a_digits-i];
    u32 b_digit = (i < b_digits) ? mem->values[RawVal(b)+b_digits-i] : 0;
    u32 sum;

    if (a_neg == b_neg) {
      sum = a_digit + b_digit + overflow;
    } else if (b_neg) {
      sum = a_digit - b_digit - overflow;
    } else {
      sum = b_digit - a_digit - overflow;
    }

    mem->values[RawVal(result)+a_digits-i] = sum;

    if (a_neg == b_neg) {
      u32 threshold = MaxUInt - a_digit;
      overflow = (overflow) ? b_digit >= threshold : b_digit > threshold;
    } else if (b_neg) {
      overflow = (overflow) ? a_digit <= b_digit : a_digit < b_digit;
    } else {
      overflow = (overflow) ? b_digit <= a_digit : b_digit < a_digit;
    }
  }

  if (overflow) {
    Val result2 = ObjVal(mem->count);
    PushMem(mem, BignumHeader(a_digits+1));
    PushMem(mem, 1);
    for (i = 0; i < a_digits; i++) PushMem(mem, mem->values[RawVal(result)+1+i]);
      return result2;
  } else {
    return result;
  }
}
