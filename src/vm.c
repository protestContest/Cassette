#include "vm.h"

const char *RegStr(Reg reg)
{
  switch (reg) {
  case REG_EXP:   return "EXP";
  case REG_ENV:   return "ENV";
  case REG_VAL:   return "VAL";
  case REG_CONT:  return "CONT";
  case REG_PROC:  return "PROC";
  case REG_ARGL:  return "ARGL";
  case REG_TMP:   return "TMP";
  default:        return "<reg>";
  }
}
