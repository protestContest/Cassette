#include "univ/res.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"

typedef struct {
  i16 resID;
  u32 len;
  char *name;
  u32 data_offset;
  void *data;
} ResRef;

typedef struct {
  u32 type;
  u16 len;
  ResRef *refs;
} ResTypeList;

typedef struct ResMap {
  u32 data_offset;
  u32 map_offset;
  struct ResMap *next;
  i32 file;
  u16 num_types;
  ResTypeList *types;
} ResMap;

static ResMap *first_res = 0;
static ResMap *current_res = 0;

i32 OpenResFile(char *path)
{
  ResMap *res;
  u32 i, j;
  u32 types_offset, names_offset;
  char *error = 0;

  res = malloc(sizeof(ResMap));
  res->file = Open(path, 0, &error);
  if (error) {
    fprintf(stderr, "%s\n", error);
    return -1;
  }

  res->num_types = 0;
  res->types = 0;
  res->next = first_res;

  Read(res->file, (char*)&res->data_offset, 4, 0);
  res->data_offset = ByteSwap(res->data_offset);
  Read(res->file, (char*)&res->map_offset, 4, 0);
  res->map_offset = ByteSwap(res->map_offset);

  Seek(res->file, res->map_offset + 24, seek_set, 0);
  Read(res->file, (char*)&types_offset, 2, 0);
  types_offset = ByteSwapShort(types_offset) + res->map_offset;
  Read(res->file, (char*)&names_offset, 2, 0);
  names_offset = ByteSwapShort(names_offset) + res->map_offset;

  Seek(res->file, types_offset, seek_set, 0);
  Read(res->file, (char*)&res->num_types, 2, 0);
  res->num_types = ByteSwapShort(res->num_types) + 1;
  res->types = malloc(sizeof(ResTypeList)*res->num_types);

  for (i = 0; i < res->num_types; i++) {
    ResTypeList type_list = {0};
    u32 refs_offset;
    Seek(res->file, types_offset + 2 + i*8, seek_set, 0);
    Read(res->file, (char*)&type_list.type, 4, 0);
    type_list.type = ByteSwap(type_list.type);
    Read(res->file, (char*)&type_list.len, 2, 0);
    type_list.len = ByteSwapShort(type_list.len) + 1;
    type_list.refs = malloc(sizeof(ResRef)*type_list.len);
    Read(res->file, (char*)&refs_offset, 2, 0);
    refs_offset = ByteSwapShort(refs_offset) + types_offset;

    Seek(res->file, refs_offset, seek_set, 0);
    for (j = 0; j < type_list.len; j++) {
      ResRef ref = {0};
      i16 name_offset;

      Seek(res->file, refs_offset + 12*j, seek_set, 0);
      Read(res->file, (char*)&ref.resID, 2, 0);
      ref.resID = ByteSwapShort(ref.resID);
      Read(res->file, (char*)&name_offset, 2, 0);
      name_offset = ByteSwapShort(name_offset);
      Seek(res->file, 1, seek_cur, 0);
      Read(res->file, ((char*)&ref.data_offset)+1, 3, 0);
      ref.data_offset = ByteSwap(ref.data_offset) + res->data_offset;

      if (name_offset >= 0) {
        u8 name_len;
        Seek(res->file, names_offset + name_offset, seek_set, 0);
        Read(res->file, (char*)&name_len, 1, 0);
        ref.name = malloc(name_len+1);
        Read(res->file, ref.name, name_len, 0);
        ref.name[name_len] = 0;
      }

      type_list.refs[j] = ref;
    }
    res->types[i] = type_list;
  }

  current_res = res;
  first_res = res;

  return res->file;
}

static void FreeResMap(ResMap *map)
{
  u32 i, j;
  for (i = 0; i < map->num_types; i++) {
    ResTypeList *type_list = &map->types[i];
    for (j = 0; j < type_list->len; j++) {
      if (type_list->refs[j].data) {
        free(type_list->refs[j].data);
      }
    }
    free(type_list->refs);
  }
  free(map->types);
  free(map);
}

void CloseResFile(i32 file)
{
  ResMap *res = current_res;
  ResMap *next;
  if (!res) return;

  if (res->file == file) {
    current_res = res->next;
    FreeResMap(res);
    return;
  }

  while (res->next && res->next->file != file) {
    res = res->next;
  }
  if (!res->next) return;

  next = res->next;
  res->next = next->next;
  FreeResMap(next);
}

void UseResFile(i32 file)
{
  ResMap *res = first_res;
  while (res && res->file != file) {
    res = res->next;
  }
  if (res) current_res = res;
}

i32 CountTypes(void)
{
  ResMap *res = first_res;
  i32 count = 0;
  while (res) {
    count += res->num_types;
    res = res->next;
  }
  return count;
}

void GetIndType(u32 *type, i32 index)
{
  ResMap *res = first_res;
  index--;
  while (res) {
    if (index < res->num_types) {
      *type = res->types[index].type;
      return;
    }
    index -= res->num_types;
    res = res->next;
  }
  *type = 0;
}

i32 CountResources(u32 type)
{
  ResMap *res = first_res;
  u32 i;
  i32 count = 0;
  while (res) {
    for (i = 0; i < res->num_types; i++) {
      if (res->types[i].type == type) {
        count += res->types[i].len;
        break;
      }
    }
    res = res->next;
  }
  return count;
}

static void *ReadResData(ResMap *res, ResRef *ref)
{
  if (!ref->data) {
    u32 len;
    char *data;
    Seek(res->file, ref->data_offset, seek_set, 0);
    Read(res->file, (char*)&len, 4, 0);
    len = ByteSwap(len);
    data = malloc(len);
    Read(res->file, data, len, 0);
    ref->len = len;
    ref->data = data;
  }
  return ref->data;
}

void *GetIndResource(u32 type, i32 index)
{
  ResMap *res = first_res;
  u32 i;
  while (res) {
    for (i = 0; i < res->num_types; i++) {
      if (res->types[i].type == type) {
        if (index < res->types[i].len) {
          return ReadResData(res, &res->types[i].refs[index]);
        }
        index -= res->types[i].len;
        break;
      }
    }
    res = res->next;
  }

  return 0;
}

void *GetResource(u32 type, i32 resID)
{
  ResMap *res = current_res;
  u32 i, j;
  while (res) {
    for (i = 0; i < res->num_types; i++) {
      if (res->types[i].type == type) {
        for (j = 0; j < res->types[i].len; j++) {
          if (res->types[i].refs[j].resID == resID) {
            return ReadResData(res, &res->types[i].refs[j]);
          }
        }
      }
    }
    res = res->next;
  }
  return 0;
}

void *GetNamedResource(u32 type, char *name)
{
  ResMap *res = current_res;
  u32 i, j;
  while (res) {
    for (i = 0; i < res->num_types; i++) {
      if (res->types[i].type == type) {
        for (j = 0; j < res->types[i].len; j++) {
          if (!res->types[i].refs[j].name) continue;
          if (StrEq(res->types[i].refs[j].name, name)) {
            return ReadResData(res, &res->types[i].refs[j]);
          }
        }
      }
    }
    res = res->next;
  }
  return 0;
}

void GetResInfo(void *data, i32 *resID, u32 *type, char **name)
{
  ResMap *res = current_res;
  u32 i, j;
  while (res) {
    for (i = 0; i < res->num_types; i++) {
      for (j = 0; j < res->types[i].len; j++) {
        if (!res->types[i].refs[j].data) continue;
        if (res->types[i].refs[j].data == data) {
          if (resID) *resID = res->types[i].refs[j].resID;
          if (type) *type = res->types[i].type;
          if (name) *name = res->types[i].refs[j].name;
          return;
        }
      }
    }
    res = res->next;
  }
  if (resID) *resID = 0;
  if (type) *type = 0;
  if (name) *name = 0;
  return;
}
