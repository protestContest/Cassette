#pragma once

#define NewVec(type, max)     ResizeVec(0, max, sizeof(type))
#define FreeVec(vec)          ((vec) ? free(RawVec(vec)),0 : 0)
#define VecCapacity(vec)      ((vec) ? RawVecCap(vec) : 0)
#define VecCount(vec)         ((vec) ? RawVecCount(vec) : 0)
#define VecPush(vec, val)     (VecMakeRoom(vec, 1), (vec)[RawVecCount(vec)++] = val)
#define VecPop(vec)           (VecCount(vec) > 0 ? (vec)[--RawVecCount(vec)] : 0)
#define GrowVec(vec, num)     (VecMakeRoom(vec, num), RawVecCount(vec) += num)
#define VecEnd(vec)           &(vec[RawVecCount(vec)])
#define VecDel(vec, i)        VecDelete(vec, i, sizeof(*(vec)))
#define VecLast(vec)          ((vec)[VecCount(vec)-1])

#define RawVec(vec)           (((u32 *)vec) - 2)
#define RawVecCap(vec)        RawVec(vec)[0]
#define RawVecCount(vec)      RawVec(vec)[1]
#define VecHasRoom(vec, n)    (VecCount(vec) + (n) <= VecCapacity(vec))
#define VecMakeRoom(vec, n)   (VecHasRoom(vec, n) ? 0 : DoVecGrow(vec, n))
#define DoVecGrow(vec, n)     (*((void **)&(vec)) = ResizeVec((vec), (n), sizeof(*(vec))/* NOLINT */))

void *ResizeVec(void *vec, u32 num_items, u32 item_size);
void VecDelete(void *vec, u32 index, u32 item_size);

#define InsertRoom(vec, i, n) \
  (VecMakeRoom(vec, n), \
  Copy(&(vec)[i], &(vec)[(i)+(n)], sizeof(*(vec))*(VecCount(vec) - (i))), \
  RawVecCount(vec) += n)
