#include <stdio.h>
#include <unistd.h>

int main() {
    write(1, "hello plz world\n", 16);
    return 0;
}
