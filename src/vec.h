#pragma once

// a vec is an array prepended with two ints in memory for capacity and count

#define NewVec(type, max)     ResizeVec(0, max, sizeof(type))
#define FreeVec(vec)          ((vec) ? free(RawVec(vec)),0 : 0)
#define VecCapacity(vec)      ((vec) ? RawVecCapacity(vec) : 0)
#define VecCount(vec)         ((vec) ? RawVecCount(vec) : 0)
#define VecPush(vec, value)   (VecMaybeGrow(vec, 1), (vec)[RawVecCount(vec)++] = value)
#define VecPop(vec)           ((vec) && VecCount(vec) > 0 ? (vec)[--RawVecCount(vec)] : nil)
#define VecEnd(vec)           &((vec)[RawVecCount(vec)])
#define VecNext(vec)          (VecMaybeGrow(vec, 1), &(vec[RawVecCount(vec)++]))
#define VecLast(vec)          (vec[VecCount(vec) - 1])
#define EmptyVec(vec)         (RawVecCount(vec) = 0)
#define RewindVec(vec, num)   RawVecCount(vec) -= (VecCount(vec) < (num) ? VecCount(vec) : (num))
#define GrowVec(vec, num)     (VecMaybeGrow(vec, num), RawVecCount(vec) += num)

#define RawVec(vec)           (((u32 *)vec) - 2)
#define UnrawVec(vec, type)   (type *)(vec + 2)
#define RawVecCapacity(vec)   RawVec(vec)[0]
#define RawVecCount(vec)      RawVec(vec)[1]
#define VecHasRoom(vec, n)    (VecCount(vec) + (n) <= VecCapacity(vec))
#define VecMaybeGrow(vec, n)  (VecHasRoom(vec, n) ? 0 : VecGrow(vec, n))
#define VecGrow(vec, n)       (*((void **)&(vec)) = ResizeVec((vec), (n), sizeof(*(vec))/* NOLINT */))

u32 *NewRawVec(u32 item_size, u32 max_items);
void *ResizeVec(void *vec, u32 num_items, u32 item_size);
void *AppendVec(void *dst, void *src, u32 item_size);
