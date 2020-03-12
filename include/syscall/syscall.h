#pragma once

#include <stdint.h>
#include <time/core.h>
#include <fs/vfs/types.h>
#include <syscall/syscall-no.h>
#include <generated/autoconf.h>

#ifdef CONFIG_CPU_32_BITS
#include <syscall/syscall32.h>
#else
#include <syscall/syscall64.h>
#endif

void syscall_exit(int status);
int syscall_yield(void);
int syscall_puts(const char *str);
int syscall_getpid(void);
int syscall_time(time_t *t);
int syscall_sleep(uint32_t secs);
int syscall_msleep(uint32_t msecs);
int syscall_usleep(uint32_t usecs);
int syscall_set_priority(uint8_t priority);
int syscall_set_process_name(char *name);
int syscall_readline(char *buf, int buflen);
int syscall_getc(void);
int syscall_backdoor(char *command, void *arg);
int syscall_getcwd(char *buf, int buflen);
int syscall_chdir(char *path);
int syscall_listdir(char *path, uint32_t offset, fs_listdir_t *dirs, int dirlen);
fs_file_t *syscall_open(char *path, fs_access_mode_t mode);
int syscall_read(fs_file_t *file, void *buf, int buflen);
int syscall_write(fs_file_t *file, void *buf, int buflen);
int syscall_close(fs_file_t *file);
int syscall_get_property(char *id, void *buf);
int syscall_spawn_process(char *executable);
