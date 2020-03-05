#include <log.h>

#include <core.h>
#include <syscall/syscall.h>
#include <property/core.h>

int syscall_get_property(char *id, void *buf) {
    return property_get(id, buf);
}
