#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <property/core.h>

int property_create(char *id, prop_mode_t mode) {
    debug("Creating property %s with mode=0x%x", id, mode);
    return -1;
}

int property_remove(char *id) {
    debug("Removing property %s", id);

    return -1;
}

int property_set(char *id, char *value) {

    return -1;
}

int property_get(char *id, char *buf) {

    return -1;
}

int property_get_int32(char *id, int32_t *buf) {

    return -1;
}



#ifdef CONFIG_TEST_CORE_PROPERTY_CORE
#include __FILE__
#endif
