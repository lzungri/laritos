#pragma once

#include <stddef.h>

#include <generated/autoconf.h>
#include "utils.h"

#ifndef KBUILD_MODNAME
#error KBUILD_MODNAME macro not found
#endif

/**
 * Adds a formatted string into the log circular buffer.
 */
int __add_log_msg(char *level, char *tag, char *fmt, ...) __attribute__((__format__(printf, 3, 4)));
int log_flush(void);

#ifdef CONFIG_LOG_FILE_AND_LINEN
#define log(_level, _msg, ...) __add_log_msg(_level, KBUILD_MODNAME, __FILE__ ":" TOSTRING(__LINE__) " - " _msg "\n", ##__VA_ARGS__)
#else
#define log(_level, _msg, ...) __add_log_msg(_level, KBUILD_MODNAME, _msg "\n", ##__VA_ARGS__)
#endif

#define fatal(_msg, ...)  do { \
    log("F", _msg, ##__VA_ARGS__); \
    while (1); \
} while(0)

#ifdef CONFIG_LOG_LEVEL_ERROR
#define error(_msg, ...) log("E", _msg, ##__VA_ARGS__)
#else
#define error(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_WARN
#define warn(_msg, ...) log("W", _msg, ##__VA_ARGS__)
#else
#define warn(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_INFO
#define info(_msg, ...) log("I", _msg, ##__VA_ARGS__)
#else
#define info(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_DEBUG
#define debug(_msg, ...) log("D", _msg, ##__VA_ARGS__)
#else
#define debug(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_VERBOSE
#define verbose(_msg, ...) log("V", _msg, ##__VA_ARGS__)
#else
#define verbose(_msg, ...)
#endif
