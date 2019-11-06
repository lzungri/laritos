#pragma once

typedef enum {
    PCB_STATUS_NOT_INIT = 0,
    PCB_STATUS_READY,
    PCB_STATUS_BLOCKED,
    PCB_STATUS_RUNNING,
    PCB_STATUS_ZOMBIE,

    PCB_STATUS_LEN,
} pcb_status_t;

static inline char *get_pcb_status_str(pcb_status_t status) {
    static char *statusstr[PCB_STATUS_LEN] =
        { "NOT INIT", "READY", "BLOCKED", "RUNNING", "ZOMBIE" };
    return status < PCB_STATUS_LEN ? statusstr[status] : "???";
}
