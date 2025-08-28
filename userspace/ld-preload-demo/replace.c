#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *changewords_3(char *sentence, char *find, char *replace) {

  char *dest = malloc(strlen(sentence) - strlen(find) + strlen(replace) + 1);
  char *ptr;

  strcpy(dest, sentence);
  ptr = strstr(dest, find);
  if (ptr) {
    memmove(ptr + strlen(replace), ptr + strlen(find),
            strlen(ptr + strlen(find)) + 1);
    strncpy(ptr, replace, strlen(replace));
  }
  return dest;
}

int main(void) {
  char *result;

  result = changewords_3("here is number one one again", "one", "two");
  printf("%s", result);
  free(result);

  return 0;
}
