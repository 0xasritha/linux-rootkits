# Peering Into the Shadows: Linux Rootkits From Red and Blue Perspectives

Rootkits have been around for decades, quietly evolving alongside the operating systems they target. On Linux, they’ve taken multiple forms—each leveraging different layers of the stack to hide processes, steal credentials, or maintain persistence. For defenders, understanding how these rootkits work isn’t just academic; it’s the first step in spotting them in the wild.

This post breaks down three major classes of Linux rootkits:

- **Userspace rootkits (LD_PRELOAD, library hijacking)**
- **Kernel rootkits (loadable kernel modules)**
- **eBPF rootkits (a newer and stealthier frontier)**

We’ll explore how each is built and deployed, and what both red teamers (attackers) and blue teamers (defenders) should know.

---

## Userspace Rootkits (LD_PRELOAD & Library Replacement)

#TODO: also include the overwriting binaries part (put code example for that if tim)
#TODO: also include the ptrace?? stuff - idk what it was called. Try to include lots of varieties of method (atleast for article)

**🔴 Red Team View**
LD_PRELOAD is the low-hanging fruit of rootkits. By preloading a malicious shared library before legitimate ones, attackers can hook libc functions like `open`, `read`, or `getdents`. With this, they can hide files, filter processes from `ps`, or even backdoor authentication routines.

Advantages:

- Simple to write (a few lines of C can hook core syscalls).
- Doesn’t require root privileges in some cases.
- Easy persistence via `~/.bashrc`, `/etc/ld.so.preload`.

**🔵 Blue Team View**
Detection strategies:

- Check `/etc/ld.so.preload` for unexpected entries.
- Run tools like `ldd` or `readelf` on suspicious binaries.
- Use integrity checkers (Tripwire, AIDE) to spot library tampering.

Limitations for attackers: LD_PRELOAD is fragile. Static binaries and containers may ignore it, and defenders can easily detect tampered libraries if they know where to look.

---

## Kernel Rootkits (Loadable Kernel Modules)

**🔴 Red Team View**
Kernel modules are the classic heavy-hitters. By inserting malicious code into the kernel, attackers can hook syscalls, hide processes, patch credentials, and bypass security modules. Unlike LD_PRELOAD, kernel rootkits operate below the visibility of userspace tools, making them far more stealthy.

Techniques:

- Syscall table hooking (`sys_call_table`).
- Direct kernel object manipulation (DKOM) to unlink processes.
- Netfilter hooks to hide backdoor traffic.

**🔵 Blue Team View**
Defenders have several angles:

- Compare `/proc/modules` vs. `lsmod` outputs for hidden modules.
- Check kernel integrity with LKRG (Linux Kernel Runtime Guard).
- Baseline kernel memory with forensic tools like Volatility.

The challenge: once an attacker is in kernel space, they’re on nearly equal footing with the OS itself. Traditional userspace monitoring often can’t see this deep.

---

## eBPF Rootkits (The Modern Stealth Layer)

**🔴 Red Team View**
eBPF (extended Berkeley Packet Filter) was designed for observability and performance monitoring—but attackers love it for the same reasons. An eBPF program can hook into kernel functions dynamically without loading a traditional module, bypassing some monitoring.

Rootkit potential:

- Hide network connections by filtering them out in eBPF maps.
- Hook process events invisibly.
- Persist stealthily, since eBPF programs don’t always appear as modules.

**🔵 Blue Team View**
Detection here is still maturing. Strategies include:

- Enumerate loaded eBPF programs (`bpftool prog show`).
- Lock down unprivileged eBPF usage (newer kernels restrict this).
- Watch for suspicious eBPF syscalls (`bpf()`) in audit logs.

eBPF rootkits are a rising threat because they blend in with legitimate observability tooling, making them perfect for living-off-the-land attacks.

---

## Red Team vs. Blue Team: A Comparative Lens

| Rootkit Type      | Red Team Advantages                                              | Blue Team Detection Strategies                             | Stealth Level        |
| ----------------- | ---------------------------------------------------------------- | ---------------------------------------------------------- | -------------------- |
| **LD_PRELOAD**    | Easy to build, works in userland, no kernel interaction required | Check `/etc/ld.so.preload`, library integrity monitoring   | Low                  |
| **Kernel Module** | Deep control, syscall hooking, process/file hiding               | LKRG, Volatility, kernel integrity checks                  | High                 |
| **eBPF**          | Modern, stealthy, blends with observability tools                | `bpftool`, audit syscall usage, restrict unprivileged eBPF | Medium-High (rising) |
