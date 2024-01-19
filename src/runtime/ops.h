#pragma once

/* Op codes are 8 bits. If the high bit is set, the op is an OpConst, with the
remaining 7 bits plus the 8 bits following the op forming an index into the
const array. If the high two bits are 01, the op is an OpInt, with the remaining
6 bits plus the 8 bits following the op forming an unsigned integer. Otherwise,
the op code identifies a normal instruction. */

#define ConstOp(op)   ((op) & 0x80)
#define IntOp(op)     (((op) & 0xC0) == 0x40)

/* const */
#define OpConst   0x80  /* push(constants[index(op)]) */
#define OpInt     0x40  /* push(int(op)) */

/* control flow */
#define OpNoop    0x00
#define OpHalt    0x01  /* halts execution */
#define OpError   0x02  /* halts execution, indicating an error (with optional message stack[0]) */
#define OpJump    0x03  /* pc <- pc + pop(stack) */
#define OpBranch  0x04  /* jumps if stack[1] is not false or nil */

/* stack */
#define OpPop     0x05  /* drop(1) */
#define OpDup     0x06  /* push(stack[0]) */
#define OpSwap    0x07  /* swaps stack[0] and stack[1] */
#define OpDig     0x08  /* stack[0] <- stack[stack[0]] */

/* obj */
#define OpStr     0x09  /* stack[0] <- symbol-name(stack[0]) */
#define OpPair    0x0A  /* stack[0] <- pair(stack[0], stack[1]); drop(1) */
#define OpNil     0x0B  /* push(nil) */
#define OpTuple   0x0C  /* stack[0] <- make-tuple(stack[0]) */
#define OpSet     0x0D  /* tuple-set(stack[2], stack[0], stack[1]); drop(2) */
#define OpMap     0x0E  /* push(make-map()) */
#define OpPut     0x0F  /* map-set(stack[2], stack[0], stack[1]); drop(2) */

/* env */
#define OpExtend  0x10  /* env <- pair(stack[0], env); drop(1) */
#define OpExport  0x11  /* push(head(env)); env <- tail(env) */
#define OpDefine  0x12  /* head(env)[stack[0]] <- stack[1]; drop(2) */
#define OpLookup  0x13  /* stack[0] <- lookup(env, stack[0]) */

/* application */
#define OpLink    0x14  /* stack[0] <- pc + stack[0]; push(env); push(link); link <- #stack */
#define OpReturn  0x15  /* link <- stack[1]; env <- stack[2]; pc <- stack[3]; stack[3] <- stack[0]; drop(3) */
#define OpLambda  0x16  /* stack[0] <- make-func(stack[0], pc + stack[1], env); drop(1) */
#define OpApply   0x17  /* application: depends on what stack[1] is (see below) */

/* Application
  num-args = pop(stack); func = pop(stack)
  primitive?(func):
    result = do-primitive(func, stack[0..num-args])
    drop(num-args)
    push(result)
  function?(func):
    pc <- func-pos(func)
    env <- func-env(func)
  num-args == 0:
    link <- pop(stack)
    env <- pop(stack)
    pc <- pop(stack)
    push(func)
  collection?(func) and num-args == 1:
    result = access(func, pop(stack))
    link <- pop(stack)
    env <- pop(stack)
    pc <- pop(stack)
    push(result)
*/

typedef u8 OpCode;
