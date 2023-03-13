// #include "image.h"
// #include <univ/vec.h>
// #include "../runtime/env.h"
// #include <univ/io.h>

// void InitImage(Image *image)
// {
//   InitModuleMap(&image->modules);
//   image->heap = NULL;
//   VecPush(image->heap, nil);
//   VecPush(image->heap, nil);
//   image->env = ExtendEnv(&image->heap, nil);
//   InitStringMap(&image->strings);
// }

// void WriteImage(Image *image, char *path)
// {
//   Print("Unimplemented");
// }

// Image *ReadImage(char *path)
// {
//   Print("Unimplemented");
//   return NULL;
// }

