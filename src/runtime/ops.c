#include "ops.h"

typedef struct {
  u8 length;
  char *name;
} OpInfo;

static OpInfo ops[] = {
  /* OpNoop */    {1, "noop"},
  /* OpHalt */    {1, "halt"},
  /* OpError */   {1, "error"},
  /* OpPop */     {1, "pop"},
  /* OpDup */     {1, "dup"},
  /* OpSwap */    {1, "swap"},
  /* OpConst */   {2, "const"},
  /* OpConst2 */  {3, "const2"},
  /* OpInt */     {2, "int"},
  /* OpNil */     {1, "nil"},
  /* OpStr */     {1, "str"},
  /* OpPair */    {1, "pair"},
  /* OpTuple */   {1, "tuple"},
  /* OpSet */     {1, "set"},
  /* OpMap */     {1, "map"},
  /* OpPut */     {1, "put"},
  /* OpExtend */  {1, "extend"},
  /* OpDefine */  {1, "define"},
  /* OpLookup */  {1, "lookup"},
  /* OpExport */  {1, "export"},
  /* OpJump */    {1, "jump"},
  /* OpBranch */  {1, "branch"},
  /* OpLambda */  {1, "lambda"},
  /* OpLink */    {1, "link"},
  /* OpReturn */  {1, "return"},
  /* OpApply */   {2, "apply"},
};

u32 OpLength(OpCode op)
{
  return ops[op].length;
}

char *OpName(OpCode op)
{
  return ops[op].name;
}
