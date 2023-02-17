#pragma once

typedef void (*WatchCallback)(char *path, void *data);

void WatchFile(char *path, WatchCallback callback, void *data);
