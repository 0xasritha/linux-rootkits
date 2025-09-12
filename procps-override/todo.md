- get my version working
- understand the stuff I didn't understand
- look at the guy's code that Om sent me, combine
- figure out how procps will get the pid to hide (file, in init function, etc.), also how it will start up
- fix global notes (move this file to the main directory)
- make a giant function hooker library for all common utils from LD_Preload? (like the evil ebpf)
- combine with the ss, same readdir?
- read through this:
  https://www.lurklurk.org/linkers/linkers.html
  kernelspace:

- look at the multiple kernel spaces ways medium article on phone
- common syscalls

---

## Repo

```
/userspace
    Makefile
    override-pid.c
    setup.py
```

**`/userspace/setup.py`**

- reverse shell
- load library in `/etc/ld.so.global`
- some way to communicate PID to hide to `/userspace/override-pid.c` (random file in tmp, environment variable, editing file directly, the program finds it by iterating through the `/proc/<pid>/cmdline` files in `init` hook, etc.)

---

# `LD_PRELOAD`

To Learn:

- `errno`

To Add:

- error handling
- initializers and deinitliazers (to get the reverse shell PID?) -> when exactly do those get run?
