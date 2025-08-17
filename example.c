// header files 
#include <linux/init.h> 
#include <linux/module.h>
#include <linux/kernel.h> 

// macros: have details about what the module does, expand into special metadata placed into kernel module's ELF section  
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Asritha Bodepudi"); 
MODULE_DESCRIPTION("Rootkit");  
MODULE_VERSION("0.01");

static int __init example_init(void) { // `__init` is macro that marks functions or data as initialization-only, letting system free that memory after initialization
    printk(KERN_INFO, "Hello World\n"); // `KERN_*` macro defines the log level of the message   
    return 0; 
}

static void __exit example_exit(void) { 
    printk(KERN_INFO, "Goodbye World!\n"); 
}

module_init(example_init); 
module_exit(example_exit); 
