#include "value.h"
#include "reader.h"
#include "printer.h"

int main(void)
{
  char *src = "(map-get {x: 1 y: 2} :y)";
  Val exp = Read(src);
  PrintTree(exp);
}
