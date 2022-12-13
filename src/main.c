#include "mem.h"
#include "reader.h"
#include "symbol.h"

int main(void)
{
  InitMem(100);
  InitSymbols();
  TokenList tokens = Read();
  PrintTokens(tokens);
  PrintMem();
  PrintSymbols();

  return 0;
}
