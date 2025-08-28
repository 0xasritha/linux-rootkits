#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* C shit - remove*/
/*
- function pointer stores address of the first instruction of a function
- can use function pointers as callbacks (path function as argument to another
function), to choose which function to run at runtime, and table of functions
(like jump tables in interpreters, state machines, etc.)
*/

/* int add(int a, int b) { return a + b; } */
/* int (*func_ptr)(int, int) = */
/*     &add; // function pointer for a function taking (int, int) and */
/*           // returning int; need () around *func_ptr to declare a function */
/*           // that returns a pointer, not a pointer to a function */
/**/
/* int (*func_ptr_2)(int, int) = add; */
/**/
/* // same: both assign a function pointer */
/* // func_ptr = &add;  // assign address of add */
/* // func_ptr_2 = add; // function name decays to a pointer */
/**/
/* int main() { */
/*   int result = func_ptr_2(2, 3); // calls add(2,3); */
/*   printf("%d", result); */

/* WRITE HOOK */

// https://man7.org/linux/man-pages/man2/write.2.html
// original write function pointer

// write() writes up to count bytes from the buffer starting at buf to the file
// referred to by the file descriptor fd
ssize_t (*original_write)(int fd, const void *buf, size_t count);

// replacement work and substitute
const char *target = "og";
const char *replacement = "new";

ssize_t write(int fd, const void *buf, size_t count) {
  // dynamically load the original write() function
  if (!original_write) {
    original_write = dlsym(RTLD_NEXT, "write");
  }

  // create a buffer to store the modified output
  char modified_buf[count + 1];
  memcpy(modified_buf, buf, count);
  modified_buf[count] = '\0'; // ensure null-terminated string

  // find and replace the target keyword
  char *pos = strstr(modified_buf, target);
  if (pos) {
    // overwrite the target with the replacement
    size_t offset = pos - modified_buf;
    memcpy(&modified_buf[offset], replacement, strlen(replacement));
  }

  // call the original write() with the modified buffer
  return original_write(fd, modified_buf, count);
}
