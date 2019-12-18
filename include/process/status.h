#pragma once

typedef enum {
    PROC_STATUS_NOT_INIT = 0,
    PROC_STATUS_READY,
    PROC_STATUS_BLOCKED,
    PROC_STATUS_RUNNING,
    PROC_STATUS_ZOMBIE,

    PROC_STATUS_LEN,
} process_status_t;

static inline char *pcb_get_status_str(process_status_t status) {
    static char *statusstr[PROC_STATUS_LEN] =
        { "NOT INIT", "READY", "BLOCKED", "RUNNING", "ZOMBIE" };
    return status < PROC_STATUS_LEN ? statusstr[status] : "???";
}
