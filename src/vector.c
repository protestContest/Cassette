#include "vector.h"
#include "printer.h"

void InitVector(Vector *vec, u32 size)
{
  vec->data = malloc(size*sizeof(Value));
  vec->size = size;
}

void ResizeVector(Vector *vec, u32 size)
{
  vec->data = realloc(vec->data, size*sizeof(Value));
  vec->size = size;
}

void VectorGrow(Vector *vec, u32 size)
{
  if (size > vec->size) {
    u32 need = vec->size + size;
    u32 new_size = (need > 2*vec->size) ? need : 2*vec->size;
    ResizeVector(vec, new_size);
  }
}

u32 VectorSize(Vector *vec)
{
  return vec->size;
}

void VectorSet(Vector *vec, u32 i, Value value)
{
  VectorGrow(vec, i + 1);

  vec->data[i] = value;
}

Value VectorGet(Vector *vec, u32 i)
{
  if (i > vec->size) {
    return nil_val;
  }

  return vec->data[i];
}

Value *VectorRef(Vector *vec, u32 i)
{
  if (i > vec->size) return NULL;
  return &vec->data[i];
}
