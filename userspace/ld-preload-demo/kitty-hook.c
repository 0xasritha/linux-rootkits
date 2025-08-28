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

/* Constructor - called when library is loaded */
__attribute__((constructor)) static void init_hook(void) {
  dbg("[hook] Library constructor called - hook is loaded!\n");
}

static void dbg(const char *fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n > 0) {
    syscall(SYS_write, 2, buf, (size_t)n); // write to stderr, no recursion
  }
}

/* Also hook __write (some programs use this) */
ssize_t __write(int fd, const void *buf, size_t count)
    __attribute__((alias("write")));

static void ensure_real_write(void) {
  if (!real_write) {
    // Debug: Show that we're being loaded
    dbg("[hook] Hook library loaded, looking for real write()...\n");

    dlerror();
    void *sym = dlsym(RTLD_NEXT, "write");
    char *err = dlerror();
    if (err || !sym) {
      dbg("[hook] dlsym(write) failed: %s\n", err ? err : "unknown");
    } else {
      real_write = (ssize_t (*)(int, const void *, size_t))sym;
      dbg("[hook] Successfully hooked write() function\n");
    }
  }
}

static char *replace_all_occurrences(const char *s, const char *find,
                                     const char *rep) {
  if (!s || !find || !rep)
    return NULL;

  const char *p = s;
  size_t count = 0;
  size_t find_len = strlen(find);
  size_t rep_len = strlen(rep);

  // Count occurrences
  while ((p = strstr(p, find)) != NULL) {
    count++;
    p += find_len;
  }

  if (count == 0) {
    return strdup(s); // No replacements needed
  }

  // Calculate new string length
  size_t old_len = strlen(s);
  size_t new_len = old_len + count * (rep_len - find_len);

  char *result = malloc(new_len + 1);
  if (!result)
    return NULL;

  char *dst = result;
  const char *src = s;

  // Perform replacements
  while ((p = strstr(src, find)) != NULL) {
    // Copy everything before the match
    size_t prefix_len = p - src;
    memcpy(dst, src, prefix_len);
    dst += prefix_len;

    // Copy replacement
    memcpy(dst, rep, rep_len);
    dst += rep_len;

    // Move source pointer past the match
    src = p + find_len;
  }

  // Copy remaining part
  strcpy(dst, src);

  return result;
}

/* Hook write() to replace "kitty" with "CHANGED-KITTY" in stdout writes */
ssize_t write(int fd, const void *buf, size_t count) {
  ensure_real_write();

  if (!real_write || !buf || count == 0 || fd != STDOUT_FILENO) {
    return real_write ? real_write(fd, buf, count) : (ssize_t)count;
  }

  /* Make a NUL-terminated copy to use string ops safely */
  char *tmp = (char *)malloc(count + 1);
  if (!tmp) {
    return real_write(fd, buf, count);
  }
  memcpy(tmp, buf, count);
  tmp[count] = '\0';

  /* Fast path: if "kitty" not present, pass-through */
  if (!strstr(tmp, "kitty")) {
    ssize_t rc = real_write(fd, buf, count);
    free(tmp);
    return rc;
  }

  dbg("[hook] Found 'kitty' in output, replacing with 'CHANGED-KITTY'\n");

  char *modified = replace_all_occurrences(tmp, "kitty", "CHANGED-KITTY");
  free(tmp);

  if (!modified) {
    return real_write(fd, buf, count);
  }

  size_t new_len = strlen(modified);
  ssize_t rc = real_write(fd, modified, new_len);
  free(modified);

  // Return the original count to satisfy the caller's expectations
  // The caller expects 'count' bytes to be "consumed"
  return (rc > 0) ? (ssize_t)count : rc;
}
