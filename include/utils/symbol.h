#pragma once

#include <log.h>
#include <stddef.h>

void *symbol_get(char *name);
int symbol_get_name_at(void *addr, char *buf, size_t blen);
