
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Prototype & lazy-load the real write() */
static ssize_t (*original_write)(int, const void *, size_t) = NULL;

static void ensure_original_write(void) {
  if (!original_write) {
    void *sym = dlsym(RTLD_NEXT, "write");
    original_write = (ssize_t (*)(int, const void *, size_t))sym;
    /* In production you’d check dlerror() here */
  }
}

/* Single-occurrence find/replace, returns newly malloc'd string */
static char *changewords_3(const char *sentence, const char *find,
                           const char *replace) {
  const char *p = strstr(sentence, find);
  if (!p) {
    /* no match: return a duplicate so caller has a uniform free() path */
    size_t n = strlen(sentence);
    char *dup = malloc(n + 1);
    if (!dup)
      return NULL;
    memcpy(dup, sentence, n + 1);
    return dup;
  }

  size_t len_s = strlen(sentence);
  size_t len_f = strlen(find);
  size_t len_r = strlen(replace);

  size_t new_len = len_s - len_f + len_r;
  char *dest = malloc(new_len + 1);
  if (!dest)
    return NULL;

  /* copy prefix */
  size_t prefix = (size_t)(p - sentence);
  memcpy(dest, sentence, prefix);
  /* copy replacement */
  memcpy(dest + prefix, replace, len_r);
  /* copy suffix */
  memcpy(dest + prefix + len_r, p + len_f, len_s - prefix - len_f);
  dest[new_len] = '\0';
  return dest;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ensure_original_write();
  if (!original_write) {
    errno = EIO;
    return -1;
  }
  if (count == 0 || buf == NULL) {
    return original_write(fd, buf, count);
  }

  /* Make a NUL-terminated copy so we can use strstr/strlen safely */
  char *tmp = malloc(count + 1);
  if (!tmp) {
    return original_write(fd, buf, count); /* fall back, no leak */
  }
  memcpy(tmp, buf, count);
  tmp[count] = '\0';

  /* Only attempt rewrite if substring is present */
  if (!strstr(tmp, "lol")) {
    ssize_t rc = original_write(fd, buf, count);
    free(tmp);
    return rc;
  }

  char *modified = changewords_3(tmp, "lol", "plz");
  if (!modified) {
    /* allocation failed: fall back */
    ssize_t rc = original_write(fd, buf, count);
    free(tmp);
    return rc;
  }

  size_t new_len = strlen(modified);
  ssize_t rc = original_write(fd, modified, new_len);

  free(modified);
  free(tmp);

  return rc;
}
