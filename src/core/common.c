#include "includes.h"
#include "vt_module.h"
#include "sync_experiment.h"
#include "utils.h"

extern int tracer_num;
extern int EXP_CPUS;
extern int TOTAL_CPUS;
extern struct task_struct ** chaintask;
extern int experiment_status;
extern int experiment_type;
extern int initialization_status;
extern s64 virt_exp_start_time;

extern struct mutex exp_lock;
extern int *per_timeline_chain_length;
extern llist * per_timeline_tracer_list;

extern s64 * tracer_clock_array;
extern s64 * aligned_tracer_clock_array;

extern hashmap get_tracer_by_id;
extern hashmap get_tracer_by_pid;

extern wait_queue_head_t expstop_call_proc_wqueue;

extern atomic_t progress_n_rounds ;
extern atomic_t n_waiting_tracers;
extern wait_queue_head_t progress_sync_proc_wqueue;
extern wait_queue_head_t * timeline_wqueue;


extern void SignalCPUWorkerResume(tracer * curr_tracer);
extern s64 GetDilatedTime(struct task_struct * task);
extern int CleanUpExperimentComponents(void);





struct task_struct* GetTaskNs(pid_t pid, struct task_struct * parent) {
    if (!parent)
        return NULL;
    int num_children = 0;
    struct list_head *list;
    struct task_struct *me;
    struct task_struct *t;
    me = parent;
    t = me;
    
    do {
        if (task_pid_nr_ns(t, task_active_pid_ns(t)) == pid)
            return t; 
    } while_each_thread(me, t);
    

    list_for_each(list, &parent->children) {
        struct task_struct * taskRecurse = list_entry(list, struct task_struct, sibling);
        if (task_pid_nr_ns(taskRecurse, task_active_pid_ns(taskRecurse)) == pid)
            return taskRecurse;
        t =  GetTaskNs(pid, taskRecurse);
        if (t != NULL)
            return t;
    }
    return NULL;
}



/***
Remove head of schedule queue and return the task_struct of the head element.
Assumes tracer write lock is acquired prior to call.
***/
int PopScheduleList(tracer * tracer_entry) {
    if (!tracer_entry)
        return 0;
    lxc_schedule_elem * head;
    head = llist_pop(&tracer_entry->schedule_queue);
    struct task_struct * curr_task;
    int pid;
    if (head != NULL) {
        pid = head->pid;
        kfree(head);
        return pid;
    }
    return 0;
}

/***
Requeue schedule queue, i.e pop from head and add to tail. Assumes tracer
write lock is acquired prior to call
***/
void RequeueScheduleList(tracer * tracer_entry) {
    if (tracer_entry == NULL)
        return;
    llist_requeue(&tracer_entry->schedule_queue);
}


/***
Remove head of schedule queue and return the task_struct of the head element.
Assumes tracer write lock is acquired prior to call.
***/
lxc_schedule_elem * PopRunQueue(tracer * tracer_entry) {
    if (!tracer_entry)
        return 0;
    return llist_pop(&tracer_entry->run_queue);
}

/***
Requeue schedule queue, i.e pop from head and add to tail. Assumes tracer
write lock is acquired prior to call
***/
void RequeueRunQueue(tracer * tracer_entry) {
    if (!tracer_entry)
        return;
    llist_requeue(&tracer_entry->run_queue);
}

/*
Assumes tracer write lock is acquired prior to call. Must return with lock
still held
*/
void CleanUpScheduleList(tracer * tracer_entry) {

    int pid = 1;
    struct pid *pid_struct;
    struct task_struct * task;

    if (tracer_entry && tracer_entry->tracer_task) {
        tracer_entry->tracer_task->associated_tracer_id = -1;
        tracer_entry->tracer_task->virt_start_time = 0;
        tracer_entry->tracer_task->curr_virt_time = 0;
        tracer_entry->tracer_task->wakeup_time = 0;
        tracer_entry->tracer_task->burst_target = 0;
    }
    while ( pid != 0) {
        pid = PopScheduleList(tracer_entry);
        if (pid) {
            pid_struct = find_get_pid(pid);
            task = pid_task(pid_struct, PIDTYPE_PID);
            if (task != NULL) {
                task->virt_start_time = 0;
                task->curr_virt_time = 0;
                task->wakeup_time = 0;
                task->burst_target = 0;
                task->associated_tracer_id = -1;
            }
        } else
            break;
    }
}

void CleanUpRunQueue(tracer * tracer_entry) {

    if (!tracer_entry)
        return;

    while (llist_size(&tracer_entry->run_queue)) {
        PopRunQueue(tracer_entry);
    }
}

/*
Assumes tracer read lock is acquired before function call
*/
int ScheduleListSize(tracer * tracer_entry) {
    if (!tracer_entry)
        return 0;
    return llist_size(&tracer_entry->schedule_queue);
}


/*
Assumes tracer read lock is acquired before function call
*/
int RunQueueSize(tracer * tracer_entry) {
    if (!tracer_entry)
        return 0;
    return llist_size(&tracer_entry->run_queue);
}



/*
Assumes tracer write lock is acquired prior to call. Must return with lock still
acquired.
*/
void AddToTracerScheduleQueue(tracer * tracer_entry,
                              int tracee_pid) {

    lxc_schedule_elem * new_elem;
    //represents base time allotted to each process by the Kronos scheduler
    //Only useful in single core mode.
    s64 base_time_quanta;
    s64 base_quanta_n_insns = 0;
    s32 rem = 0;
    struct sched_param sp;
    struct task_struct * tracee = FindTaskByPid(tracee_pid);

    if (!tracee) {
        PDEBUG_E("AddToTracerScheduleQueue: tracee: %d not found!\n", tracee_pid);
        return;
    }
    
    if (experiment_status == STOPPING) {
        PDEBUG_E("AddToTracerScheduleQueue: cannot add when experiment is stopping !\n");
        return;
    }

    PDEBUG_I("AddToTracerScheduleQueue: "
             "Adding new tracee %d to tracer-id: %d\n",
             tracee->pid, tracer_entry->tracer_id);

    new_elem = (lxc_schedule_elem *)kmalloc(sizeof(lxc_schedule_elem), GFP_KERNEL);
    
    if (!new_elem) {
        PDEBUG_E("AddToTracerScheduleQueue: "
                 "Tracer %d, tracee %d. Failed to alot Memory\n",
                 tracer_entry->tracer_id, tracee->pid);
        return;
    }
    memset(new_elem, 0, sizeof(lxc_schedule_elem));

    new_elem->pid = tracee->pid;
    new_elem->curr_task = tracee;
    tracee->associated_tracer_id = tracer_entry->tracer_id;

    new_elem->base_quanta = PROCESS_MIN_QUANTA_NS;
    new_elem->quanta_left_from_prev_round = 0;
    new_elem->quanta_curr_round = 0;
    new_elem->blocked = 0;

        

    if (tracer_entry->tracer_type == TRACER_TYPE_APP_VT) {
        BUG_ON(!aligned_tracer_clock_array);
        BUG_ON(!chaintask[tracer_entry->timeline_assignment]);
        tracee->tracer_clock = (s64 *)&aligned_tracer_clock_array[tracee->associated_tracer_id - 1];
        tracee->vt_exec_task = chaintask[tracer_entry->timeline_assignment];
        tracee->ready = 0;
        tracee->ptrace_msteps = 0;
        tracee->n_ints = 0;
    } else {
        //tracee->vt_exec_task_wqueue = NULL;
        tracee->vt_exec_task = NULL;
        tracee->ready = 0;
    }

    if (!tracee->virt_start_time) {
        tracee->virt_start_time = virt_exp_start_time;
        tracee->curr_virt_time = tracer_entry->curr_virtual_time;
        tracee->wakeup_time = 0;
        tracee->burst_target = 0;
    }
    

    tracee->assigned_timeline = tracer_entry->timeline_assignment;

    llist_append(&tracer_entry->schedule_queue, new_elem);


    bitmap_zero((&tracee->cpus_allowed)->bits, 8);
    cpumask_set_cpu(tracer_entry->cpu_assignment, &tracee->cpus_allowed);
    sp.sched_priority = 99;
    sched_setscheduler(tracee, SCHED_RR, &sp);
    PDEBUG_A("AddToTracerScheduleQueue:  "
             "Tracee %d added successfully to tracer-id: %d. schedule list size: %d\n",
             tracee->pid, tracer_entry->tracer_id, ScheduleListSize(tracer_entry));
}



/*
Assumes tracer write lock is acquired prior to call. Must return with lock still
acquired.
*/
void RemoveFromTracerScheduleQueue(tracer * tracer_entry, int tracee_pid) 
{

    lxc_schedule_elem * curr_elem;
    lxc_schedule_elem * removed_elem;
    struct task_struct * tracee = FindTaskByPid(tracee_pid);

    llist_elem * head_1;
    llist_elem * head_2;
    int pos = 0;

    

    PDEBUG_I("RemoveFromTracerScheduleQueue: "
             "Removing tracee %d from tracer-id: %d\n",
             tracee_pid, tracer_entry->tracer_id);

    head_1 = tracer_entry->schedule_queue.head;
    head_2 = tracer_entry->run_queue.head;

    if (tracee) {
        tracee->associated_tracer_id = -1;
        tracee->virt_start_time = 0;
        tracee->curr_virt_time = 0;
        tracee->wakeup_time = 0;
        tracee->burst_target = 0;
    }

    while (head_1 != NULL || head_2 != NULL) {

        if (head_1) {
            curr_elem = (lxc_schedule_elem *)head_1->item;
            if (curr_elem && curr_elem->pid == tracee_pid) {
                removed_elem = llist_remove_at(&tracer_entry->schedule_queue, pos);
            }
            head_1 = head_1->next;
        }

        if (head_2) {
            curr_elem = (lxc_schedule_elem *)head_2->item;
            if (curr_elem && curr_elem->pid == tracee_pid) {
                llist_remove_at(&tracer_entry->run_queue, pos);
            }
            head_2 = head_2->next;
        }
        pos ++;
    }

    if (removed_elem) 
        kfree(removed_elem);
}


/**
* write_buffer: <tracer_id>,<tracer_type>,<meta_task_type>,<timeline_id>,<spinner_pid> or <proc_to_control_pid>
**/
int RegisterTracerProcess(char * write_buffer) {

    tracer * new_tracer;
    uint32_t tracer_id;
    int i;
    int best_cpu = 0;
    int assigned_cpu;
    int meta_task_type = 0;
    int tracer_type;
    int spinner_pid = 0, process_to_control_pid = 0;
    struct task_struct * spinner_task = NULL;
    struct task_struct * process_to_control = NULL;
    int api_args[MAX_API_ARGUMENT_SIZE];
    int num_args;
    int assigned_timeline_id;


    num_args = ConvertStringToArray(write_buffer, api_args, MAX_API_ARGUMENT_SIZE);

    BUG_ON(num_args <= 3);

    tracer_type = api_args[1];
    tracer_id = api_args[0];
    meta_task_type = api_args[2];
    assigned_timeline_id = api_args[3];

    if (tracer_type != TRACER_TYPE_INS_VT && tracer_type != TRACER_TYPE_APP_VT) {
        PDEBUG_E("Unknown Tracer Type !");
        return FAIL;
    }

    if (meta_task_type == 1) {
        BUG_ON(num_args != 5);
        spinner_pid  = api_args[4];
        if (spinner_pid <= 0) {
            PDEBUG_E("Spinner pid must be greater than zero. "
                     "Received value: %d\n", spinner_pid);
            return FAIL;
        }

        spinner_task = GetTaskNs(spinner_pid, current);

        if (!spinner_task) {
            PDEBUG_E("Spinner pid task not found. "
                     "Received pid: %d\n", spinner_pid);
            return FAIL;
        }


    } else if (meta_task_type != 0) {
        meta_task_type = 0;
        BUG_ON(num_args != 5);
        process_to_control_pid = api_args[4];
        if (process_to_control_pid <= 0) {
            PDEBUG_E("proccess_to_control_pid must be greater than zero. "
                     "Received value: %d\n", process_to_control_pid);
            return FAIL;
        }

        process_to_control = FindTaskByPid(process_to_control_pid);
        if (!process_to_control) {
            PDEBUG_E("proccess_to_control_pid task not found. "
                     "Received pid: %d\n", process_to_control_pid);
            return FAIL;
        }
    }

    mutex_lock(&exp_lock);
    ++tracer_num;
    if (tracer_id == -1) {
        tracer_id = tracer_num;
    }
    mutex_unlock(&exp_lock);
    
    PDEBUG_I("Register Tracer: Starting for tracer: %d\n", tracer_id);

    new_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);

    if (!new_tracer) {
        new_tracer = AllocTracerEntry(tracer_id, tracer_type);
        hmap_put_abs(&get_tracer_by_id, tracer_id, new_tracer);
        hmap_put_abs(&get_tracer_by_pid, current->pid, new_tracer);
    } else {
        InitializeTracerEntry(new_tracer, tracer_id, tracer_type);
        hmap_put_abs(&get_tracer_by_pid, current->pid, new_tracer);
    }

    if (!new_tracer)
        return FAIL;

    mutex_lock(&exp_lock);

    if (experiment_type == EXP_CS) {
        assigned_cpu = (assigned_timeline_id % EXP_CPUS);

    } else {
        for (i = 0; i < EXP_CPUS; i++) {
            if (per_timeline_chain_length[i]
                < per_timeline_chain_length[best_cpu])
                best_cpu = i;
        }
        assigned_timeline_id = best_cpu;
        assigned_cpu = best_cpu;
        per_timeline_chain_length[best_cpu] ++;
    }

    
    new_tracer->tracer_id = tracer_id;
    new_tracer->cpu_assignment = assigned_cpu;
    new_tracer->timeline_assignment = assigned_timeline_id;
    new_tracer->tracer_task = current;
    new_tracer->create_spinner = meta_task_type;
    new_tracer->proc_to_control_task = NULL;
    new_tracer->spinner_task = NULL;
    new_tracer->w_queue = &timeline_wqueue[new_tracer->timeline_assignment];
    new_tracer->tracer_pid = current->pid;

    BUG_ON(!chaintask[new_tracer->timeline_assignment]);
    new_tracer->vt_exec_task = chaintask[new_tracer->timeline_assignment];

    current->associated_tracer_id = tracer_id;
    current->ready = 0;
    current->ptrace_msteps = 0;
    current->n_ints = 0;
    current->ptrace_mflags = 0; 
    current->vt_exec_task = NULL;
    current->assigned_timeline = assigned_timeline_id;

    
    llist_append(&per_timeline_tracer_list[assigned_timeline_id], new_tracer);
    mutex_unlock(&exp_lock);

    if (meta_task_type) {
        PDEBUG_I("Register Tracer: Pid: %d, ID: %d "
         "assigned cpu: %d, assigned timeline: %d, spinner pid = %d\n",
         current->pid, new_tracer->tracer_id,
         new_tracer->cpu_assignment, new_tracer->timeline_assignment,
         spinner_pid);

    } 

    if (process_to_control != NULL) {
        PDEBUG_I("Register Tracer: Pid: %d, ID: %d "
        "assigned cpu: %d, assigned timeline: %d process_to_control pid = %d\n",
        current->pid, new_tracer->tracer_id,
        new_tracer->cpu_assignment, new_tracer->timeline_assignment,
        process_to_control_pid);

    }

    bitmap_zero((&current->cpus_allowed)->bits, 8);

    GetTracerStructWrite(new_tracer);
    cpumask_set_cpu(new_tracer->cpu_assignment, &current->cpus_allowed);
    

    if (meta_task_type && spinner_task) {
        PDEBUG_I("Set Spinner Task for Tracer: %d, Spinner: %d\n",
                 current->pid, spinner_task->pid);
        new_tracer->spinner_task = spinner_task;
        spinner_task->assigned_timeline = assigned_timeline_id;
        KillProcess(spinner_task, SIGSTOP);
        bitmap_zero((&spinner_task->cpus_allowed)->bits, 8);
        cpumask_set_cpu(1, &spinner_task->cpus_allowed);

    } 
    
    if (!meta_task_type && process_to_control != NULL) {
        process_to_control->assigned_timeline = assigned_timeline_id;
        AddToTracerScheduleQueue(new_tracer, process_to_control_pid);
        new_tracer->proc_to_control_pid = process_to_control_pid;
        new_tracer->proc_to_control_task = process_to_control;
    } else {
        new_tracer->proc_to_control_pid = -1;
    }

    PutTracerStructWrite(new_tracer);
    PDEBUG_I("Register Tracer: Finished for tracer: %d. Assigned timeline: %d\n", tracer_id, new_tracer->timeline_assignment);
    return new_tracer->cpu_assignment;	//return the allotted cpu back to the tracer.
}



/*
Assumes tracer write lock is acquired prior to call. Must return with write lock
still acquired.
*/
void UpdateAllChildrenVirtualTime(tracer * tracer_entry, s64 time_increment) {


    if (tracer_entry && tracer_entry->tracer_task) {

        tracer_entry->round_start_virt_time += time_increment;
        tracer_entry->curr_virtual_time = tracer_entry->round_start_virt_time;

        if (ScheduleListSize(tracer_entry) > 0)
            SetChildrenTime(tracer_entry, tracer_entry->tracer_task,
                                tracer_entry->curr_virtual_time, 0);

        if (tracer_entry->spinner_task)
            tracer_entry->spinner_task->curr_virt_time =
                tracer_entry->curr_virtual_time;

        if (tracer_entry->proc_to_control_task != NULL) {
            if (FindTaskByPid(tracer_entry->proc_to_control_pid) != NULL)
                tracer_entry->proc_to_control_task->curr_virt_time =
                    tracer_entry->curr_virtual_time;
        }

    }

}


void UpdateInitTaskVirtualTime(s64 time_to_set) {
    init_task.curr_virt_time = time_to_set;
}

/*
Assumes no tracer lock is acquired prior to call
*/
void UpdateAllTracersVirtualTime(int timelineID) {
    llist_elem * head;
    llist * tracer_list;
    tracer * curr_tracer;
    s64 overshoot_err;
    s64 target_increment;

    tracer_list =  &per_timeline_tracer_list[timelineID];
    head = tracer_list->head;


    while (head != NULL) {

        curr_tracer = (tracer*)head->item;
        GetTracerStructWrite(curr_tracer);
        target_increment = curr_tracer->nxt_round_burst_length;

        if (target_increment > 0) {
            if (curr_tracer && curr_tracer->tracer_type == TRACER_TYPE_APP_VT) {
                BUG_ON(!aligned_tracer_clock_array);
                //WARN_ON(aligned_tracer_clock_array[curr_tracer->tracer_id - 1] < curr_tracer->round_start_virt_time +  target_increment);
                if (aligned_tracer_clock_array[curr_tracer->tracer_id - 1] < curr_tracer->round_start_virt_time +  target_increment) {
                    // this can happen if no process belonging to tracer was runnable in the previous round.
                    aligned_tracer_clock_array[curr_tracer->tracer_id - 1] = curr_tracer->round_start_virt_time +  target_increment;
                }
                overshoot_err = aligned_tracer_clock_array[curr_tracer->tracer_id - 1] - (curr_tracer->round_start_virt_time +  target_increment);
                curr_tracer->round_overshoot = overshoot_err;
                target_increment = target_increment + overshoot_err;
                UpdateAllChildrenVirtualTime(curr_tracer, target_increment);

                WARN_ON(aligned_tracer_clock_array[curr_tracer->tracer_id - 1] != curr_tracer->curr_virtual_time);
            } else {
                UpdateAllChildrenVirtualTime(curr_tracer, target_increment);
            }
        } else {
            PDEBUG_E("Tracer nxt round burst length is 0\n");
        }
        curr_tracer->nxt_round_burst_length = 0;
        PutTracerStructWrite(curr_tracer);
        head = head->next;
    }
}





/**
* write_buffer: result which indicates overflow number of instructions.
  It specifies the total number of instructions by which the tracer overshot
  in the current round. The overshoot is ignored if experiment type is CS.
  Assumes no tracer lock is acquired prior to call.
**/

int HandleTracerResults(tracer * curr_tracer, int * api_args, int num_args) {

    struct pid *pid_struct;
    struct task_struct * task;
    int i, pid_to_remove;
    int wakeup = 0;

    if (!curr_tracer)
        return FAIL;

    GetTracerStructWrite(curr_tracer);
    for (i = 0; i < num_args; i++) {
        pid_to_remove = api_args[i];
        if (pid_to_remove <= 0)
            break;

        PDEBUG_I("Handle tracer results: Pid: %d, Tracer ID: %d, "
                 "Ignoring Process: %d\n", curr_tracer->tracer_task->pid,
                 curr_tracer->tracer_id, pid_to_remove);
        struct task_struct * mappedTask = GetTaskNs(pid_to_remove, curr_tracer->tracer_task);
        if (curr_tracer->tracer_type == TRACER_TYPE_APP_VT) {
            WARN_ON(mappedTask && mappedTask->pid != current->pid);
            if (mappedTask && mappedTask->pid == current->pid && current->burst_target > 0) {
                wakeup = 1;
            }
        }
        if (mappedTask != NULL) {
            RemoveFromTracerScheduleQueue(curr_tracer, mappedTask->pid);
        }

        
    }

    PutTracerStructWrite(curr_tracer);

    if (wakeup) {
        PDEBUG_V("APPVT Tracer signalling resume. Tracer ID: %d\n",
                curr_tracer->tracer_id);
        curr_tracer->w_queue_wakeup_pid = 1;
        //wake_up_interruptible(curr_tracer->w_queue);
        wake_up_process(curr_tracer->vt_exec_task);

    } else {
        SignalCPUWorkerResume(curr_tracer);
    }
    return SUCCESS;
}

void WaitForInsVTTracerCompletion(tracer * curr_tracer)
{

    if (!curr_tracer)
        return;
    BUG_ON(curr_tracer->tracer_type != TRACER_TYPE_INS_VT);
    curr_tracer->w_queue_wakeup_pid = curr_tracer->tracer_pid;
    wake_up_interruptible(curr_tracer->w_queue);
    wait_event_interruptible(*curr_tracer->w_queue, curr_tracer->w_queue_wakeup_pid == 1);
    PDEBUG_V("Resuming from Tracer completion for Tracer ID: %d\n", curr_tracer->tracer_id);

}

void WaitForAppVTTracerCompletion(tracer * curr_tracer, struct task_struct * relevant_task)
{
    if (!curr_tracer || !relevant_task) {
        return;
    }
    int ret;
    BUG_ON(curr_tracer->tracer_type != TRACER_TYPE_APP_VT);
    PDEBUG_V("Waiting for Tracer completion for Tracer ID: %d\n", curr_tracer->tracer_id);
    curr_tracer->w_queue_wakeup_pid = relevant_task->pid;
    set_current_state(TASK_INTERRUPTIBLE);
    wake_up_interruptible(curr_tracer->w_queue);
    //wait_event_interruptible(*curr_tracer->w_queue, relevant_task->burst_target == 0 || curr_tracer->w_queue_wakeup_pid == 1);
    
    /*do {
        ret = wait_event_interruptible_timeout(
                  *curr_tracer->w_queue, relevant_task->burst_target == 0 || curr_tracer->w_queue_wakeup_pid == 1, HZ);
        if (ret == 0)
            set_current_state(TASK_INTERRUPTIBLE);
        else
            set_current_state(TASK_RUNNING);

    } while (ret == 0);*/
    do {
        
        if (relevant_task->burst_target == 0 || curr_tracer->w_queue_wakeup_pid == 1) {
            set_current_state(TASK_RUNNING);
            relevant_task->burst_target = 0;
            curr_tracer->w_queue_wakeup_pid = 1;
            break;
        } else {
            //set_current_state(TASK_INTERRUPTIBLE);
            schedule();
            set_current_state(TASK_INTERRUPTIBLE);
        }
    } while (1);
    PDEBUG_V("Resuming from Tracer completion for Tracer ID: %d\n", curr_tracer->tracer_id);

}

void SignalCPUWorkerResume(tracer * curr_tracer) {

    if (!curr_tracer)
        return;

    if (curr_tracer->tracer_type == TRACER_TYPE_INS_VT) {
        PDEBUG_V("INSVT Tracer signalling resume. Tracer ID: %d\n",
                 curr_tracer->tracer_id);

        curr_tracer->w_queue_wakeup_pid = 1;
        wake_up_interruptible(curr_tracer->w_queue);
    } else {
        if (current->burst_target > 0) {
            PDEBUG_V("APPVT Tracer signalling resume. Tracer ID: %d\n",
                curr_tracer->tracer_id);
            curr_tracer->w_queue_wakeup_pid = 1;
            //wake_up_interruptible(curr_tracer->w_queue);
            wake_up_process(curr_tracer->vt_exec_task);
        }
    }

}


int HandleStopExpCmd() {
    // Any process can invoke this call.
    if (initialization_status != INITIALIZED) {
        PDEBUG_I(
            "VT_STOP_EXP: Operation cannot be performed when experiment is not "
            "initialized !");
        return -EFAULT;
    }

    if (experiment_status == NOTRUNNING) {
        PDEBUG_I(
            "VT_STOP_EXP: Operation cannot be performed when experiment is not "
            "running!");
        return -EFAULT;
    }

    experiment_status = STOPPING;
    InitiateExperimentStopOperation();
    return CleanUpExperimentComponents();
}

int HandleInitializeExpCmd(char * buffer) {
    int num_expected_tracers;
    int exp_type;
    int num_timelines;
    int api_args[MAX_API_ARGUMENT_SIZE];
    int num_args = ConvertStringToArray(buffer, api_args,
                      MAX_API_ARGUMENT_SIZE);

    BUG_ON(num_args < 3);

    exp_type = api_args[0];
    num_timelines = api_args[1];
    num_expected_tracers = api_args[2];

    if (num_expected_tracers) {
        return InitializeExperimentComponents(exp_type, num_timelines,
            num_expected_tracers);
    }

    return FAIL;
}


/**
* write_buffer: <tracer_id>,<network device name>
* Can be called after successfull synchronize and freeze command
**/
int HandleSetNetdeviceOwnerCmd(char * write_buffer) {


    char dev_name[IFNAMSIZ];
    int tracer_id;
    struct pid * pid_struct = NULL;
    int i = 0;
    struct net * net;
    struct task_struct * task;
    tracer * curr_tracer;
    int found = 0;
    int next_idx = 0;

    for (i = 0; i < IFNAMSIZ; i++)
	dev_name[i] = '\0';


    tracer_id = atoi(write_buffer);
    next_idx = GetNextValue(write_buffer);
    

    for (i = 0; * (write_buffer + next_idx + i) != '\0'
	&& *(write_buffer + next_idx + i) != ','  && i < IFNAMSIZ ; i++)
	dev_name[i] = *(write_buffer + next_idx + i);


    if (!tracer_id) 
	PDEBUG_A("Set Net Device Owner: Network Interface Name: %s. Attempting to attach to any tracer..\n",
	        dev_name);
    else
	PDEBUG_A("Set Net Device Owner: Network Interface Name:: %s. Attempting to attach to tracer: %d\n",
		 dev_name, tracer_id);

    struct net_device * dev;

    if (!tracer_id) {
	for (i = 1; i <= tracer_num; i++) {
	     curr_tracer = hmap_get_abs(&get_tracer_by_id, i);
	     if (curr_tracer && curr_tracer->spinner_task)
		break;
	}
    } else
	curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
	

    if (!curr_tracer) {
	PDEBUG_E("No suitable tracer found. Failed to set Net Device Owner for: %s\n", dev_name);
	return FAIL;
    }

    task = curr_tracer->spinner_task;
    if (!task) {
        PDEBUG_E("Must have spinner task to "
                 "be able to set net device owner\n");
        return FAIL;
    }

    pid_struct = get_task_pid(task, PIDTYPE_PID);
    if (!pid_struct) {
        PDEBUG_E("Pid Struct of spinner task not found for tracer: %d\n",
                  curr_tracer->tracer_task->pid);
        return FAIL;
    }

    write_lock_bh(&dev_base_lock);
    for_each_net(net) {
        for_each_netdev(net, dev) {
            if (dev != NULL) {
                if (strcmp(dev->name, dev_name) == 0 && !found) {
                    PDEBUG_A("Set Net Device Owner: "
                             "Configured Specified Net Device: %s\n", dev_name);
                    dev->owner_pid = pid_struct;
                    found = 1;
                } else if (found) {
                    break;
                }
            }
        }
    }
    write_unlock_bh(&dev_base_lock);
    return SUCCESS;
}


s64 GetDilatedTime(struct task_struct * task) {
    struct timeval tv;
    do_gettimeofday(&tv);
    s64 now = timeval_to_ns(&tv);

    if (task->virt_start_time != 0) {

        if (task->tracer_clock != NULL) {
            return *(task->tracer_clock);
        }
        return task->curr_virt_time;
    }

    return now;

}

/**
* write_buffer: <pid> of process
**/
s64 HandleGettimePID(char * write_buffer) {

    struct pid *pid_struct;
    struct task_struct * task;
    int pid;

    pid = atoi(write_buffer);

    PDEBUG_V("Handle gettimepid: Received Pid = %d\n", pid);

    task = FindTaskByPid(pid);
    if (!task)
        return 0;
    return GetDilatedTime(task);
}


