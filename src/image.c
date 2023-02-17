#include "image.h"
#include "vec.h"
#include "mem.h"

void InitImage(Image *image)
{
  image->header[0] = '\0';
  image->header[1] = 'r';
  image->header[2] = 'y';
  image->header[3] = 'e';

  InitModuleMap(&image->modules);
  image->heap = NULL;
  VecPush(image->heap, nil);
  VecPush(image->heap, nil);
  image->env = MakePair(&image->heap, nil, nil);
  InitStringMap(&image->strings);
}

void WriteImage(Image *image, char *path)
{
  printf("Unimplemented");
}

Image *ReadImage(char *path)
{
  printf("Unimplemented");
  return NULL;
}

