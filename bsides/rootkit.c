#include <kallsyms.h>
// Syscall Table Hijacking

unsigned long cr0;
extern unsigned long __force_order;

static inline void write_forced_cr0(unsigned long val) {
  asm volatile("mov %0, %%cr0" : "+r"(val), "+m"(__force_order));
}

/*
- what is `pr_info`? 

*/

static int __init chs_init(void) {
  pr_info("chs: Entering: %s\n", __func__);
  sys_call_table = (void *)kallsyms_lookup_name("sys_call_table");
  if (!sys _call_table) {
    pr_err("chs: could not look up the sys call table\n");
    return -1;
  }

  pr_info("chs: sys_call_table [%p]\n", sys_call_table);
  syscall_hijack();
}

static void syscall_hijack(void) { orig_read = (typeof(ksys_read) *)sys_call_table[__NR_read]; }
