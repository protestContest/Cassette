#pragma once

i32 OpenResFile(char *path);
void CloseResFile(i32 file);
void UseResFile(i32 file);
i32 CountTypes(void);
void GetIndType(u32 *type, i32 index);
i32 CountResources(u32 type);
void *GetIndResource(u32 type, i32 index);
void *GetResource(u32 type, i32 resID);
void *GetNamedResource(u32 type, char *name);
void GetResInfo(void *res, i32 *resID, u32 *type, char **name);
