#pragma once

#include "elf.h"
#include "disk.h"

enum {
    PROC_UNUSED,
    PROC_LOADING, /* allocated and wait for loading elf binary */
    PROC_READY,   /* finished loading elf and wait for first running */
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_WAIT_TO_SEND,
    PROC_WAIT_TO_RECV
};

struct process{
    int pid;
    int status;
    int receiver_pid; /* used when waiting to send a message */
    void *sp, *mepc;  /* process context = stack pointer (sp)
                       * + machine exception program counter (mepc) */
};

#define MAX_NPROCESS  16
extern int proc_curr_idx;
extern struct process proc_set[MAX_NPROCESS];
#define curr_pid      proc_set[proc_curr_idx].pid
#define curr_status   proc_set[proc_curr_idx].status

void timer_init();
void timer_reset();

void proc_init();
int  proc_alloc();
void ps();
void proc_free(int);
void proc_set_ready (int);
void proc_set_running (int);
void proc_set_runnable (int);

void ctx_entry(void);
void ctx_start(void** old_sp, void* new_sp);
void ctx_switch(void** old_sp, void* new_sp);
