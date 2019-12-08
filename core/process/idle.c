#include <log.h>

#include <process/core.h>


int idle_main(void *data) {
    info("DATA: 0x%p", data);
    while (1);

    return 0;
}
