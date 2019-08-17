#pragma once

#include <generated/autoconf.h>
#include <prep-utils.h>

#ifndef MODULE
#error MODULE macro not found. It must be defined in the file using the log infraestructure
#endif

/**
 * Adds a formatted string into the log circular buffer.
 */
int __add_log_msg(char *level, char *tag, char *fmt, ...) __attribute__((__format__(printf, 3, 4)));

#ifdef CONFIG_LOG_FILE_AND_LINEN
#define log(_level, _msg, ...) __add_log_msg(_level, MODULE, __FILE__ ":" TOSTRING(__LINE__) " - " _msg "\n", ##__VA_ARGS__)
#else
#define log(_level, _msg, ...) __add_log_msg(_level, MODULE, _msg "\n", ##__VA_ARGS__)
#endif

#define fatal(_msg, ...)  do { \
    log("FATAL", _msg, ##__VA_ARGS__); \
    while (1); \
} while(0)

#ifdef CONFIG_LOG_LEVEL_ERROR
#define error(_msg, ...) log("ERROR", _msg, ##__VA_ARGS__)
#else
#define error(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_WARN
#define warn(_msg, ...) log("WARN", _msg, ##__VA_ARGS__)
#else
#define warn(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_INFO
#define info(_msg, ...) log("INFO", _msg, ##__VA_ARGS__)
#else
#define info(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_DEBUG
#define debug(_msg, ...) log("DBG ", _msg, ##__VA_ARGS__)
#else
#define debug(_msg, ...)
#endif

#ifdef CONFIG_LOG_LEVEL_VERBOSE
#define verbose(_msg, ...) log("VERB", _msg, ##__VA_ARGS__)
#else
#define verbose(_msg, ...)
#endif
