Assignment 3: System Information Fetching Kernel Module
===
- Assignment 3: System Information Fetching Kernel Module
  - Linux Kernel Module
  - Descriptions
  - Kernel Module: kfetch_mod
    - Kfetch information mask
    - Device operations
  - Requirements
  	- Default logo
  	
In this assignment, you are going to implement a kernel module that fetches the system information from the kernel, like below screenshot:

![image](https://hackmd.io/_uploads/BJHxzRrRT.png)

Implement a kernel module fetches system info from kernel
---
**Step 0: Create new program**
```
vim kfetch_mod_312551002.c
```

**Step 1: Implement the fetches system information function**
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>       // For si_meminfo
#include <linux/utsname.h>
#include <linux/timekeeping.h>
#include <linux/sched.h>
#include <linux/sched/stat.h>
#include <linux/cpu.h>
#include <linux/sched/signal.h>
#include <linux/utsname.h>

#define DEVICE_NAME "kfetch_mod_312551002"
#define CLASS_NAME "kfetch"
#define BUFFER_SIZE 1024

// Information mask definitions
#define KFETCH_NUM_INFO 6
#define KFETCH_RELEASE   (1 << 0)
#define KFETCH_NUM_CPUS  (1 << 1)
#define KFETCH_CPU_MODEL (1 << 2)
#define KFETCH_MEM       (1 << 3)
#define KFETCH_UPTIME    (1 << 4)
#define KFETCH_NUM_PROCS (1 << 5)
#define KFETCH_FULL_INFO ((1 << KFETCH_NUM_INFO) - 1)

static int majorNumber;
static struct class *kfetchClass = NULL;
static struct cdev kfetchCdev;
static int mask_info = KFETCH_FULL_INFO;
static char data_buffer[BUFFER_SIZE];

static int dev_open(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static int dev_release(struct inode *, struct file *);

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init kfetch_init(void) {
    printk(KERN_INFO "Kfetch: Initializing the Kfetch Module\n");
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "Kfetch failed to register a major number\n");
        return majorNumber;
    }

    printk(KERN_INFO "Kfetch: Registered with major number %d\n", majorNumber);
    kfetchClass = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(kfetchClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(kfetchClass);
    }

    if (IS_ERR(device_create(kfetchClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME))) {
        class_destroy(kfetchClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(device_create(kfetchClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME));
    }

    cdev_init(&kfetchCdev, &fops);

    if (cdev_add(&kfetchCdev, MKDEV(majorNumber, 0), 1) < 0) {
        device_destroy(kfetchClass, MKDEV(majorNumber, 0));
        class_destroy(kfetchClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to add cdev\n");
        return -1;
    }

    printk(KERN_INFO "Kfetch: Device class created correctly\n");
    return 0;
}

static void __exit kfetch_exit(void) {
    cdev_del(&kfetchCdev);
    device_destroy(kfetchClass, MKDEV(majorNumber, 0));
    class_destroy(kfetchClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Kfetch: Module successfully unloaded\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Kfetch: Device has been opened\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Kfetch: Device successfully closed\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    struct sysinfo si;
    struct timespec64 uptime;
    unsigned long mem_free_mb, mem_total_mb, uptime_seconds;
    int num_procs = 0;
    struct task_struct *task;
    char *info = NULL;
    char *formatted_output = NULL;
    char *info_ptr;
    char *line;
    char *next_info;
    char *next_line;
    int i;

    char hostname[65]; // HOST_NAME_MAX typically is 64
    char separator_line[65]; // Same size as hostname for simplicity
    const char *logo_lines[] = {
        "       \e[33mLinux\e[0m        ",
        "        .-.        ",
        "       (.. |       ",
        "       \e[33m<>\e[0m  |        ",
        "      / --- \\      ",
        "     ( |   | |     ",
        "   \e[33m|\\\e[0m\\_)___/\\)\e[33m/\\\e[0m    ",
        "  \e[33m<__)\e[0m------\e[33m(__/\e[0m    "
    };
    size_t hostname_len;

    // Initialize the hostname and separator_line
    memset(hostname, 0, sizeof(hostname));
    strncpy(hostname, init_uts_ns.name.nodename, sizeof(hostname) - 1);
    hostname_len = strlen(hostname);
    memset(separator_line, '-', hostname_len);
    separator_line[hostname_len] = '\0';

    // Clear the buffer
    memset(data_buffer, 0, BUFFER_SIZE);

    // Gather system information
    si_meminfo(&si);
    ktime_get_boottime_ts64(&uptime);
    for_each_process(task) if (task->mm) num_procs++;

    mem_free_mb = (si.freeram * si.mem_unit) / 1024 / 1024;
    mem_total_mb = (si.totalram * si.mem_unit) / 1024 / 1024;
    uptime_seconds = uptime.tv_sec;

    // Allocate info buffer dynamically
    info = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    formatted_output = kmalloc(BUFFER_SIZE, GFP_KERNEL);

    if (!info || !formatted_output) {
        kfree(info); // It's safe to call kfree with NULL
        kfree(formatted_output);
        return -ENOMEM;
    }

    // Prepare the info string with proper padding for each line
    info_ptr = info;
    info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33m%s\e[0m\n%s\n", hostname, separator_line);

    // Concatenate information based on the mask_info
    if (mask_info & KFETCH_RELEASE || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mKernel:\e[0m  %-20s\n", utsname()->release);
    }

    if (mask_info & KFETCH_CPU_MODEL || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mCPU:\e[0m     %-20s\n", boot_cpu_data.x86_model_id);
    }

    if (mask_info & KFETCH_NUM_CPUS || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mCPUs:\e[0m    %d / %-14d\n", num_online_cpus(), num_possible_cpus());
    }

    if (mask_info & KFETCH_MEM || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mMem:\e[0m     %lu MB / %lu MB\n", mem_free_mb, mem_total_mb);
    }

    if (mask_info & KFETCH_NUM_PROCS || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mProcs:\e[0m   %-d\n", num_procs);
    }

    if (mask_info & KFETCH_UPTIME || mask_info == KFETCH_FULL_INFO) {
        info_ptr += snprintf(info_ptr, BUFFER_SIZE - (info_ptr - info), "\e[33mUptime:\e[0m  %-lu mins\n", uptime_seconds / 60);
    }

    // Formatting the output to align side by side with the logo
    next_info = info;
    line = formatted_output;

    for (i = 0; i < ARRAY_SIZE(logo_lines) || next_info; ++i) {
        if (i < ARRAY_SIZE(logo_lines)) {
            line += sprintf(line, "%-20s", logo_lines[i]);
        } else {
            line += sprintf(line, "%-20s", "");
        }

        if (next_info) {
            next_line = strchr(next_info, '\n');

            if (next_line) *next_line = '\0'; // Terminate the current line
            line += sprintf(line, " %s\n", next_info);
            next_info = next_line ? next_line + 1 : NULL;
        } else {
            line += sprintf(line, "\n");
        }
    }

    // Copy the formatted buffer to user space
    if (copy_to_user(buffer, formatted_output, strlen(formatted_output))) {
        kfree(info);
        kfree(formatted_output);
        return -EFAULT; // Failed to copy to user space
    }

    // Free the dynamically allocated buffers
    kfree(info);
    kfree(formatted_output);

    // Return the number of characters sent
    return strlen(formatted_output);
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    // Update mask_info based on user input
    if (len == sizeof(int)) {
        if (copy_from_user(&mask_info, buffer, sizeof(int))) {
            printk(KERN_ERR "Kfetch: Error setting mask_info\n");
            return -EFAULT;
        }

        printk(KERN_INFO "Kfetch: Mask set to %d\n", mask_info);

        return sizeof(int);
    } else {
        printk(KERN_ERR "Kfetch: Incorrect mask size\n");
        return -EINVAL;
    }
}

module_init(kfetch_init);
module_exit(kfetch_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benedict");
MODULE_DESCRIPTION("System Information Fetching Kernel Module");
MODULE_VERSION("0.1");
```
**Step 3: Create new Makefile**
```
vim Makefile
```

**Step 4: Implement a Makefile**
```c
obj-m += kfetch_mod_312551002.o

DEVICE_NAME := kfetch_mod_312551002

# Use shell to grab the major number dynamically. This will be empty if the device is not loaded.
MAJOR_NUMBER := $(shell grep $(DEVICE_NAME) /proc/devices | cut -d ' ' -f 1)

all: build unload remove_device_node load create_device_node

build:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

load: 
	# Check if module is already loaded, if yes, remove it
	@if lsmod | grep $(DEVICE_NAME) &> /dev/null ; then \
		echo "Module $(DEVICE_NAME) is already loaded, unloading first..."; \
		sudo rmmod $(DEVICE_NAME) || true; \
	fi
	# Insert the module
	sudo insmod $(DEVICE_NAME).ko


create_device_node:
	# Check if device node already exists, if not, create it
	@if [ ! -e /dev/$(DEVICE_NAME) ] ; then \
		echo "Creating device node /dev/$(DEVICE_NAME)..."; \
		sudo mknod /dev/$(DEVICE_NAME) c $(MAJOR_NUMBER) 0 ; \
		sudo chmod 666 /dev/$(DEVICE_NAME) ; \
	else \
		echo "Device node /dev/$(DEVICE_NAME) already exists."; \
	fi

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	# Remove the module if it is loaded
	@if lsmod | grep $(DEVICE_NAME) &> /dev/null ; then \
		echo "Unloading module $(DEVICE_NAME)..."; \
		sudo rmmod $(DEVICE_NAME) || true; \
	else \
		echo "Module $(DEVICE_NAME) is not loaded."; \
	fi

remove_device_node:
	# Remove the device node if it exists
	@if [ -e /dev/$(DEVICE_NAME) ] ; then \
		echo "Removing device node /dev/$(DEVICE_NAME)..."; \
		sudo rm -f /dev/$(DEVICE_NAME) ; \
	else \
		echo "Device node /dev/$(DEVICE_NAME) does not exist."; \
	fi

.PHONY: all build load create_device_node clean unload remove_device_node
```

**Step 5: Compile written program**
```
make
```

**Step 6: Compile test program**
We have prepared the user-space program kfetch for you. Source code `kfetch.c` and the header file (shared with the kernel module) `kfetch.h` to test your module.
```
cc 。、kfetch.c -o kfetch
```

**Step 7: Test the program**
```
sudo ./kfetch -h
```
![image](https://hackmd.io/_uploads/Hy7-MABRT.png)

**Step 8: Test the program**
Initially, when the module is loaded, the first invocation without any options will display all the information. If the options -c and m are specified, only the information about the CPU model name and the memory will be displayed.
```
sudo ./kfetch -a
```
![image](https://hackmd.io/_uploads/ByxfGArAp.png)

```
sudo ./kfetch -m -c
```
![image](https://hackmd.io/_uploads/B19GzCBCa.png)