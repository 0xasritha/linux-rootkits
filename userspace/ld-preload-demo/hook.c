#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static ssize_t (*real_write)(int, const void *, size_t) = NULL;

static void dbg(const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n > 0) {
    syscall(SYS_write, 2, buf, (size_t)n); // write to stderr, no recursion
  }
}

static void ensure_real_write(void) {
  if (!real_write) {
    dlerror();
    void *sym = dlsym(RTLD_NEXT, "write");
    char *err = dlerror();
    if (err || !sym) {
      dbg("[hook] dlsym(write) failed: %s\n", err ? err : "unknown");
    } else {
      real_write = (ssize_t (*)(int, const void *, size_t))sym;
    }
  }
}

static char *replace_once(const char *s, const char *find, const char *rep) {
  const char *p = strstr(s, find);
  if (!p)
    return strdup(s); // uniform ownership
  size_t len_s = strlen(s), len_f = strlen(find), len_r = strlen(rep);
  size_t new_len = len_s - len_f + len_r;
  char *out = (char *)malloc(new_len + 1);
  if (!out)
    return NULL;
  size_t pre = (size_t)(p - s);
  memcpy(out, s, pre);
  memcpy(out + pre, rep, len_r);
  memcpy(out + pre + len_r, p + len_f, len_s - pre - len_f);
  out[new_len] = '\0';
  return out;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ensure_real_write();
  if (!real_write || !buf || count == 0 || fd != STDOUT_FILENO) {
    return real_write ? real_write(fd, buf, count) : (ssize_t)count;
  }

  // Debug: show we're intercepting
  dbg("[hook] Intercepted write to stdout, count=%zu\n", count);

  /* Make a NUL-terminated copy to use string ops safely */
  char *tmp = (char *)malloc(count + 1);
  if (!tmp) {
    return real_write(fd, buf, count);
  }
  memcpy(tmp, buf, count);
  tmp[count] = '\0';

  /* Fast path: if "lol" not present, pass-through */
  if (!strstr(tmp, "lol")) { // Changed from "plz" to "lol"
    ssize_t rc = real_write(fd, buf, count);
    free(tmp);
    return rc;
  }

  dbg("[hook] Found 'lol', replacing with 'plz'\n");
  char *mod = replace_once(tmp, "lol", "plz"); // Changed replacement
  free(tmp);
  if (!mod) {
    return real_write(fd, buf, count);
  }
  size_t new_len = strlen(mod);
  ssize_t rc = real_write(fd, mod, new_len);
  free(mod);
  return rc;
}
