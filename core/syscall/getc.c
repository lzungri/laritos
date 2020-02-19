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
