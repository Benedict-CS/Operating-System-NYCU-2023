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