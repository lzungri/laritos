#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <generated/autoconf.h>
#include <utils/utils.h>


#ifndef KBUILD_MODNAME
#error KBUILD_MODNAME macro not found
#endif

#ifdef CONFIG_LOG_USE_COLORS
#define RESTORE_COLOR "\e[39m"
#define FATAL_COLOR "\e[38;5;196m"
#define ERROR_COLOR "\e[91m"
#define WARN_COLOR "\e[93m"
#define INFO_COLOR "\e[92m"
#define DEBUG_COLOR "\e[94m"
#define VERBOSE_COLOR "\e[38;5;244m"
#else
#define RESTORE_COLOR ""
#define FATAL_COLOR ""
#define ERROR_COLOR ""
#define WARN_COLOR ""
#define INFO_COLOR ""
#define DEBUG_COLOR ""
#define VERBOSE_COLOR ""
#endif

/**
 * Adds a formatted string into the log circular buffer.
 *
 * @param sync: Indicates whether or not the call to this function should block (as a
 *      consequence of flushing the log message for example)
 * @param level: F(atal), E(rror), W(arning), I(nfo), D(ebug), V(erbose)
 * @param tag: Module identifier
 * @param fmt: Message
 * @param ...: varargs
 */
int __add_log_msg(bool sync, char *level, char *tag, char *fmt, ...) __attribute__((__format__(printf, 4, 5)));
int log_flush(void);

#ifdef CONFIG_LOG_FILE_AND_LINEN
#define log(_sync, _level, _msg, ...) __add_log_msg(_sync, _level, KBUILD_MODNAME, __FILE__ ":" TOSTRING(__LINE__) " " _msg "\n", ##__VA_ARGS__)
#else
#define log(_sync, _level, _msg, ...) __add_log_msg(_sync, _level, KBUILD_MODNAME, _msg "\n", ##__VA_ARGS__)
#endif

#define log_always(_msg, ...) log(true, INFO_COLOR "I", _msg RESTORE_COLOR, ##__VA_ARGS__)
#define log_always_async(_msg, ...) log(false, INFO_COLOR "I", _msg RESTORE_COLOR, ##__VA_ARGS__)

#define fatal_sync(_sync, _msg, ...)  do { \
        log(_sync, FATAL_COLOR "F", _msg RESTORE_COLOR, ##__VA_ARGS__); \
        while (1) { \
            arch_wfi(); \
        } \
    } while(0)

#define fatal(_msg, ...) fatal_sync(true, _msg, ##__VA_ARGS__)
#define fatal_async(_msg, ...) fatal_sync(false, _msg, ##__VA_ARGS__)


#if defined(CONFIG_LOG_LEVEL_ERROR) || defined(DEBUG)
#define error_sync(_sync, _msg, ...) log(_sync, ERROR_COLOR "E", _msg RESTORE_COLOR, ##__VA_ARGS__)
#else
#define error_sync(_sync, _msg, ...)
#endif

#define error(_msg, ...) error_sync(true, _msg, ##__VA_ARGS__)
#define error_async(_msg, ...) error_sync(false, _msg, ##__VA_ARGS__)


#if defined(CONFIG_LOG_LEVEL_WARN) || defined(DEBUG)
#define warn_sync(_sync, _msg, ...) log(_sync, WARN_COLOR "W", _msg RESTORE_COLOR, ##__VA_ARGS__)
#else
#define warn_sync(_sync, _msg, ...)
#endif

#define warn(_msg, ...) warn_sync(true, _msg, ##__VA_ARGS__)
#define warn_async(_msg, ...) warn_sync(false, _msg, ##__VA_ARGS__)


#if defined(CONFIG_LOG_LEVEL_INFO) || defined(DEBUG)
#define info_sync(_sync, _msg, ...) log(_sync, INFO_COLOR "I", _msg RESTORE_COLOR, ##__VA_ARGS__)
#else
#define info_sync(_sync, _msg, ...)
#endif

#define info(_msg, ...) info_sync(true, _msg, ##__VA_ARGS__)
#define info_async(_msg, ...) info_sync(false, _msg, ##__VA_ARGS__)


#if defined(CONFIG_LOG_LEVEL_DEBUG) || defined(DEBUG)
#define debug_sync(_sync, _msg, ...) log(_sync, DEBUG_COLOR "D", _msg RESTORE_COLOR, ##__VA_ARGS__)
#else
#define debug_sync(_sync, _msg, ...)
#endif

#define debug(_msg, ...) debug_sync(true, _msg, ##__VA_ARGS__)
#define debug_async(_msg, ...) debug_sync(false, _msg, ##__VA_ARGS__)


#if defined(CONFIG_LOG_LEVEL_VERBOSE) || defined(DEBUG)
#define verbose_sync(_sync, _msg, ...) log(_sync, VERBOSE_COLOR "V", _msg RESTORE_COLOR, ##__VA_ARGS__)
#else
#define verbose_sync(_sync, _msg, ...)
#endif

#define verbose(_msg, ...) verbose_sync(true, _msg, ##__VA_ARGS__)
#define verbose_async(_msg, ...) verbose_sync(false, _msg, ##__VA_ARGS__)
