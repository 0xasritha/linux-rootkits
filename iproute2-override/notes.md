# iproute2

## `misc/ss.c`

- `ss` used to dump socket statistics
  - no options: displays list of open non-listening sockets (ex. TCP, UNIX, UDP) with established connection (meaning sockets actively in use for communication, not those just waiting for connections (LISTEN))
  - pulls data from `procfs` and netlink APIs
    - `/proc`:
      - `/proc/net/tcp`: IPv4 TCP sockets
      - `/proc/net/tcp6`: IPv6 TCP sockets
      - `/proc/net/udp`: IPv4 UDP sockets
      - `/proc/net/udp6`: IPv6 UDP sockets
      - `/proc/net/unix`: UNIX domain sockets
      - `/proc/net/raw`/`raw6`: raw sockets
      - `/proc/net/packet`: packet sockets

### Code breakdown

- after line 482 - 498:
  - bunch of #define net_xx_open macros

- `static void user_ent_hash_build_task(char *path, int pid, int tid)`
  - line 550

- `static void user_ent_hash_build(void)`
  - line 643
  - walks down `/proc` filesystem (or alt. directory if `PROC_ROOT`) is set to build data structure of processes and (optionally threads)
    - for each directory where directory's name is a number (so pid), call `user_ent_hash_build_task` with `(path, pid, task)` where `tid == pid` (main thread of process)
    - if tasks enabled: TODO
