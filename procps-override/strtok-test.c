#include <stdio.h>
#include <string.h>

int main() {

  /* 
   * `char *strtok(char *str, const char *delim)`
   * breaks string `str` into a series of tokens using the delimeter
   * returns a pointer to the first token found in the string, returns a null pointer if there are no more tokens left to retrieve 
   */

  char path[250] = "/proc/123/status/test.txt";
  const char delim[2] = "/";
  char *token;

  /* 
   * First Call: 
   * - `strtok` scans `path` for first token separated by `/`
   * - modifies `path` in place by replacing the first delimeter it finds with `\0`, so token becomes a standalone string 
   * - saves it's position (right after that delimeter in a hidden static pointer) inside `strtok`
   */

  token = strtok(path, delim); // get first token

  // walk through other tokens
  while (token != NULL) {
    printf("%s\n", token);
    /*
     * Subsequent Calls: 
     * - `strtok` maintains internal state between calls, passing `NULL` tells `strtok` "continue from where you left off"
     * - uses it's saved static pointer to find the next token 
     * - each call moves that pointer forward and returns the next token until no more delimeters are found (then it returns `NULL`)
     */
    token = strtok(NULL, delim);
  }

  return 0;
}
