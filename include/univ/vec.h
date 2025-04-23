#pragma once

#define NewVec(type, max)     ResizeVec(0, max, sizeof(type))
#define FreeVec(vec)          ((vec) ? free(RawVec(vec)),0 : 0)
#define VecCapacity(vec)      ((vec) ? RawVecCap(vec) : 0)
#define VecCount(vec)         ((vec) ? RawVecCount(vec) : 0)
#define VecPush(vec, val)     (VecMakeRoom(vec, 1), (vec)[RawVecCount(vec)++] = val)
#define VecPop(vec)           ((vec)[--RawVecCount(vec)])
#define GrowVec(vec, num)     (VecMakeRoom(vec, Max(1, num)), RawVecCount(vec) += num)
#define VecEnd(vec)           &(vec[RawVecCount(vec)])
#define VecTrunc(vec,n)       ((vec) ? RawVecCount(vec) = (n),0 : 0)
#define VecDelete(vec,i)      (DoVecDelete((u8*)vec, i, sizeof(*(vec))), RawVecCount(vec)--)

#define RawVec(vec)           (((u32 *)vec) - 2)
#define RawVecCap(vec)        RawVec(vec)[0]
#define RawVecCount(vec)      RawVec(vec)[1]
#define VecHasRoom(vec, n)    (VecCount(vec) + (n) <= VecCapacity(vec))
#define VecMakeRoom(vec, n)   (VecHasRoom(vec, n) ? 0 : DoVecGrow(vec, n))
#define DoVecGrow(vec, n)     (*((void **)&(vec)) = ResizeVec((vec), (n), sizeof(*(vec))/* NOLINT */))

void *ResizeVec(void *vec, u32 numItems, u32 itemSize);
void DoVecDelete(u8 *vec, u32 index, u32 itemSize);
