# Creating shared library

gcc -fPIC -g -c -Wall testls.c

gcc -shared -Wl,-soname,testls.so \
 -o testls.so testls.o -lc

export LD_PRELOAD=$PWD/testls.so

---

# strace

- `strace ls`: runs `ls` but shows every syscall it makes
- `strace -e write ls`: only show `write`
- `strace -c ls`: count syscalls
- `strace -o trace.log /bin/ls`: dump to a file instead of screen
- `strace -o trace.log -s 200 /bin/ls`: dump to a file
  - `-s 200`: increase max string size to avoid `...` (`strace` truncates long strings, prints ~32 bytes by default)
