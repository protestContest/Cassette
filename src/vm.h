#pragma once

typedef enum {
  REG_EXP,
  REG_ENV,
  REG_VAL,
  REG_CONT,
  REG_PROC,
  REG_ARGL,
  REG_TMP,
  NUM_REGS
} Reg;

const char *RegStr(Reg reg);
