#include <log.h>

#include <string.h>
#include <process/core.h>
#include <syscall/syscall.h>
#include <component/component.h>
#include <component/inputdev.h>

int syscall_getc(void) {
    inputdev_t *input = component_get_default(COMP_TYPE_INPUTDEV, inputdev_t);
    if (input == NULL) {
        error("No input device available");
        return -1;
    }

    char buf[1];
    int nbytes = input->transport->ops.read(input->transport, buf, sizeof(buf), true);
    return nbytes < 1 ? nbytes : buf[0];
}

int syscall_puts(const char *s) {
    // Output process message as a raw string (i.e. no date, pid, tag metadata, etc)
    __add_log_msg(true, NULL, NULL, (char *) s);
    return 0;
}

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
