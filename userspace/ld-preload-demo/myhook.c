// myhook.c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count) {
  // Get the real write
  static ssize_t (*real_write)(int, const void *, size_t) = NULL;
  if (!real_write)
    real_write = dlsym(RTLD_NEXT, "write");

  // Custom behavior
  const char *prefix = "[hooked] ";
  real_write(fd, prefix, strlen(prefix));

  // Call the real one
  return real_write(fd, buf, count);
}
