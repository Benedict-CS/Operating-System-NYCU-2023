Assignment 1: Compiling Linux Kernel and Adding Custom System Calls
===
- Assignment 1: Compiling Linux Kernel and Adding Custom System Calls
  - Compiling Linux Kernel
    - `Change kernel suffix`
  - Adding custom system calls
    - `sys_hello`
    - `sys_revstr`

Part 1: Compiling Linux Kernel & Change kernel suffix
---
**Step 1: Download and Extract the Linux kernel source code**
```
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.19.12.tar.xz
```
```
tar -xJf linux-5.19.12.tar.xz
```
```
cd linux-5.19.12
```

**Step 2: Install essential tools and libraries required for compiling the kernel**
```
sudo apt update
sudo apt upgrade
sudo apt-get install flex
sudo apt-get install bison
sudo apt install build-essential
sudo apt install libelf-dev libssl-dev
sudo apt-get install libncurses5-dev gcc make git exuberant-ctags bc libssl-dev
```

**Step 3: Copy current kernel config file, then set it as default configuration**
```
cp /boot/config-`uname -r`* .config
```
```
make defconfig
```

**Step 4: Modify the config file**
```
vim .config
```

Find `CONFIG_LOCALVERSION` change value to `-os-312551002`

**Step 5: Compile the kernel (wait 20~50 mins)**
```
make -j4
```

**Step 6: Install the kernel modules**
```
sudo make modules_install install
```

**Step 7: Update GRUB, the bootloader**
```
sudo update-grub2
```

**Step 8: Reboot the system**
```
reboot
```
Press **Esc** button when system boot > Advanced options for Ubuntu > choose `Linux 5.19.12-os-312551002` to start

**Step 9: Check the running kernel version**
```
uname -a
```
**The screenshot of changed kernel suffix**
![image](https://hackmd.io/_uploads/rkmobCrAT.png)

Part 2: Adding custom system calls
---
### `sys_hello`
For the implementation of the sys_hello system calls, I <font color="#f00">modified 4 kernel source files</font> and <font color="#f00">added 1 new C code</font> to verify if the system calls work correctly. The modified and added files are:
1. linux-5.19.12/kernel/<font color="#f00">sys.c</font>
2. linux-5.19.12/arch/x86/include/generated/uapi/asm/<font color="#f00">unistd_64.h</font>
3. linux-5.19.12/arch/x86/entry/syscalls/<font color="#f00">syscall_64.tbl</font>
4. linux-5.19.12/include/linux/<font color="#f00">syscalls.h</font>
5. <font color="#f00">sys_hello.c</font>

**Step 1: In the sys.c file, add the following code to defines a new system call named "hello".**
```c
SYSCALL_DEFINE0(hello){
	printk(KERN_INFO "Hello, world! \n");
	printk(KERN_INFO "312551002 \n");
	return 0;
}
```
**Step 2: In the unistd_64.h file, add the following code to defines the system call number for "hello".**
```
#define __NR_hello 548
```
**Step 3: In the syscall_64.tbl file, add the following entry to map system call number to its corresponding function.**
```
548	common	hello	sys_hello
```
**Step 4: In the syscalls.h file, add the following code to add the new system call in the header file.**
```
asmlinkage long sys_hello(void);
```
**Step 5: Create new C code file named sys_hello.c and add the following code:**
```c
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_hello 548

int main(int argc, char *argv[]) {
    int ret = syscall(__NR_hello);
    assert(ret == 0);

    return 0;
}
```
**Step 6: Rebuild the kernel to compile the modified kernel source code.**
```
make
```
**Step 7: Reinstall the kernel**
```
sudo make modules_install install
```
**Step 8: Reboot the kernel**
```
reboot
```
**Step 9: Compile the sys_hello.c file to test if the system call operates correctly.**
```
gcc sys_hello.c -o sys_hello
```
```
./sys_hello
```
**Step 10: Display the output messages produced by the sys_hello system call.**
```
dmesg | tail
```

**The screenshot of the results of executing sys_hello system call:**
![image](https://hackmd.io/_uploads/BkbnZCrCp.png)

### `sys_revstr`
For the implementation of the sys_hello system calls, I <font color="#f00">modified 4 kernel source files</font> and <font color="#f00">added 1 new C code</font> to verify if the system calls work correctly. The modified and added files are:
1. linux-5.19.12/kernel/<font color="#f00">sys.c</font>
2. linux-5.19.12/arch/x86/include/generated/uapi/asm/<font color="#f00">unistd_64.h</font>
3. linux-5.19.12/arch/x86/entry/syscalls/<font color="#f00">syscall_64.tbl</font>
4. linux-5.19.12/include/linux/<font color="#f00">syscalls.h</font>
5. <font color="#f00">sys_revstr.c</font>

**Step 1: In the sys.c file, add the following code to defines a new system call named "revstr".**
```c
SYSCALL_DEFINE2(revstr, int, len, char __user *, usr_str) {
    char k_str[256];
    char k_revstr[256];
    int start = 0;
    int end = len - 1;

    if(len >= sizeof(k_str))
        return -EINVAL;
    if(copy_from_user(k_str, usr_str, len))
        return -EFAULT;

    k_str[len] = '\0';
    printk(KERN_INFO "The origin string: %s", k_str);

    while(start < end) {
        char temp = k_str[start];
        k_str[start] = k_str[end];
        k_str[end] = temp;
        start++;
        end--;
    }

    strncpy(k_revstr, k_str, sizeof(k_str));
    printk(KERN_INFO "The reversed string: %s", k_revstr);

    return 0;
}
```

**Step 2: In the unistd_64.h file, add the following code to defines the system call number for "revstr".**
```
#define __NR_revstr 549
```
**Step 3: In the syscall_64.tbl file, add the following entry to map system call number to its corresponding function.**
```
549	common	revstr	sys_revstr
```
**Step 4: In the syscalls.h file, add the following code to add the new system call in the header file.**
```
asmlinkage long sys_revstr(int len, char __user *usr_str);
```
**Step 5: Create new C code file named sys_revstr.c and add the following code:**
```
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_revstr 549

int main(int argc, char *argv[]) {  
    int ret1 = syscall(__NR_revstr, 5, "hello");
    assert(ret1 == 0);

    int ret2 = syscall(__NR_revstr, 11, "5Y573M C411");
    assert(ret2 == 0);
    return 0;
}
```
**Step 6: Rebuild the kernel to compile the modified kernel source code.**
```
make
```
**Step 7: Reinstall the kernel**
```
sudo make modules_install install
```
**Step 8: Reboot the kernel**
```
reboot
```
**Step 9: Compile the sys_revstr.c file to test if the system call operates correctly.**
```
gcc sys_revstr.c -o sys_revstr
```
```
./sys_revstr
```
**Step 10: Display the output messages produced by the sys_ revstr system call.**
```
dmesg | tail
```
**The screenshot of the results of executing sys_ revstr system call:**
![image](https://hackmd.io/_uploads/HJn3WAHR6.png)