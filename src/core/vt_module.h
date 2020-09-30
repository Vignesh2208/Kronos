#ifndef __VT_MODULE_H
#define __VT_MODULE_H

#include "includes.h"


#define NOTRUNNING -1
#define RUNNING 0
#define FROZEN 1
#define STOPPING 2
#define NOT_INITIALIZED 0
#define INITIALIZED 1
#define DILATION_FILE "kronos"

#define MAX_API_ARGUMENT_SIZE 100
#define BUF_MAX_SIZE MAX_API_ARGUMENT_SIZE


typedef struct overshoot_info_struct {
    s64 round_error;
    s64 n_rounds;
    s64 round_error_sq;
} overshoot_info;


typedef struct invoked_api_struct {
    char api_argument[MAX_API_ARGUMENT_SIZE];
    s64 return_value;
} invoked_api;

typedef struct sched_queue_element {
    s64 base_quanta;
    s64 quanta_left_from_prev_round;
    s64 quanta_curr_round;
    int pid;
    int blocked;
    struct task_struct * curr_task;
} lxc_schedule_elem;


typedef struct tracer_struct {
    struct task_struct * tracer_task;
    struct task_struct * spinner_task;
    struct task_struct * proc_to_control_task;
    struct task_struct * vt_exec_task;

    int proc_to_control_pid;
    int create_spinner;
    int tracer_id;
    int tracer_pid;
    int tracer_type;
    u32 timeline_assignment;
    u32 cpu_assignment;
    s64 curr_virtual_time;
    s64 round_overshoot;
    s64 round_start_virt_time;
    s64 nxt_round_burst_length;

    rwlock_t tracer_lock;

    llist schedule_queue;
    llist run_queue;
    wait_queue_head_t * w_queue;
    int w_queue_wakeup_pid;
    lxc_schedule_elem * last_run;
} tracer;


typedef struct timeline_struct {
    int timeline_id;
    s64 nxt_round_burst_length;
    s64 curr_virtual_time;
    int status;
    int stopping;
    wait_queue_head_t * w_queue;
} timeline;



#endif
