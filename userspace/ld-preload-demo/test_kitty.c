#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    const char *msg = "Process name: kitty terminal\n";
    write(1, msg, strlen(msg));
    return 0;
}
