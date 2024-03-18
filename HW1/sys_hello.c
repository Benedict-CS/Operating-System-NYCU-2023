#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_hello 548

int main(int argc, char *argv[]) {
    int ret = syscall(__NR_hello);
    assert(ret == 0);

    return 0;
}