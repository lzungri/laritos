#include <log.h>

#include <process/core.h>
#include <syscall/syscall.h>

void syscall_exit(int status) {
    process_exit(status);
    // Execution will never reach this point
}
