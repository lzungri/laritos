#include <log.h>

#include <string.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <component/component.h>
#include <component/inputdev.h>

int syscall_readline(char *buf, int buflen) {
    inputdev_t *input = component_get_default(COMP_TYPE_INPUTDEV, inputdev_t);
    if (input == NULL) {
        error("No input device available");
        return -1;
    }

    char *ptr = buf;
    int remaining = buflen;
    while (remaining > 1) {
        int bread = input->transport->ops.read(input->transport, ptr, remaining - 1, true);
        if (bread < 0) {
            error("Failed to read from '%s' input device", input->parent.id);
            return -1;
        }

        ptr[bread] = '\0';

        char *newline = strchr(ptr, '\r');
        if (newline != NULL) {
            *newline = '\0';
            remaining -= newline - ptr;
            break;
        }

        ptr += bread;
        remaining -= bread;
    }

    return buflen - remaining;
}
