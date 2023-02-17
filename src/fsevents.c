#include "fsevents.h"
#include <sys/event.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct {
  char *path;
  int queue;
  WatchCallback callback;
} WatchInfo;

static void *WatchLoop(void *arg)
{
  WatchInfo *info = (WatchInfo*)arg;

  printf("Watching \"%s\"\n", info->path);

  while (true) {
    struct kevent change;
    if (kevent(info->queue, NULL, 0, &change, 1, NULL) == -1) {
      exit(1);
    }

    info->callback(info->path, change.udata);
  }
}

void WatchFile(char *path, WatchCallback callback, void *data)
{
  int queue = kqueue();
  int dir = open(path, O_RDONLY);

  WatchInfo *info = malloc(sizeof(WatchInfo));
  info->path = path;
  info->queue = queue;
  info->callback = callback;

  struct kevent event;
  EV_SET(&event, dir, EVFILT_VNODE, EV_ADD | EV_CLEAR | EV_ENABLE, NOTE_WRITE, 0, data);
  kevent(queue, &event, 1, NULL, 0, NULL);

  pthread_t thread;
  if (pthread_create(&thread, NULL, WatchLoop, info)) {
    printf("Thread error\n");
    exit(1);
  }
}
