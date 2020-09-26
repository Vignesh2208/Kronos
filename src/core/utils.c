#include "utils.h"


extern int experiment_status;
extern s64 virt_exp_start_time;
extern int tracer_num;
extern int ScheduleListSize(tracer * tracer_entry);
extern hashmap get_tracer_by_id;
extern struct mutex exp_lock;



char * AllocMmapPages(int npages)
{
    int i;
    char *mem = kmalloc(PAGE_SIZE * npages, GFP_KERNEL);

    if (!mem)
        return mem;

    memset(mem, 0, PAGE_SIZE * npages);
    for(i = 0; i < npages * PAGE_SIZE; i += PAGE_SIZE) {
        SetPageReserved(virt_to_page(((unsigned long)mem) + i));
    }

    return mem;
}

void FreeMmapPages(void *mem, int npages)
{
    int i;
    if (!mem)
        return;

    for(i = 0; i < npages * PAGE_SIZE; i += PAGE_SIZE) {
        ClearPageReserved(virt_to_page(((unsigned long)mem) + i));
    }

    kfree(mem);
}


/***
Used when reading the input from a userland process.
Will basically return the next number in a string
***/
int GetNextValue (char *write_buffer) {
    int i;
    for (i = 1; * (write_buffer + i) >= '0'
            && *(write_buffer + i) <= '9'; i++) {
        continue;
    }
    return (i + 1);
}

/***
 Convert string to integer
***/
int atoi(char *s) {
    int i, n;
    n = 0;
    for (i = 0; * (s + i) >= '0' && *(s + i) <= '9'; i++)
        n = 10 * n + *(s + i) - '0';
    return n;
}


/***
Given a pid, returns a pointer to the associated task_struct
***/
struct task_struct* FindTaskByPid(unsigned int nr) {
    struct task_struct* task;
    rcu_read_lock();
    task = pid_task(find_vpid(nr), PIDTYPE_PID);
    rcu_read_unlock();
    return task;
}

void InitializeTracerEntry(tracer * new_tracer, uint32_t tracer_id, int tracer_type) {

    if (!new_tracer)
        return;
        
    new_tracer->proc_to_control_pid = -1;
    new_tracer->cpu_assignment = 0;
    new_tracer->timeline_assignment = 0;
    new_tracer->tracer_id = tracer_id;
    new_tracer->tracer_pid = 0;
    new_tracer->round_start_virt_time = 0;
    new_tracer->nxt_round_burst_length = 0;
    new_tracer->curr_virtual_time = 0;
    new_tracer->round_overshoot = 0;
    new_tracer->tracer_type = tracer_type;
    new_tracer->w_queue_wakeup_pid = 0;
    new_tracer->last_run = NULL;

    llist_destroy(&new_tracer->schedule_queue);
    llist_destroy(&new_tracer->run_queue);

    llist_init(&new_tracer->schedule_queue);
    llist_init(&new_tracer->run_queue);	

    rwlock_init(&new_tracer->tracer_lock);


}

tracer * AllocTracerEntry(uint32_t tracer_id, int tracer_type) {

    tracer * new_tracer = NULL;

    new_tracer = (tracer *)kmalloc(sizeof(tracer), GFP_KERNEL);
    if (!new_tracer)
        return NULL;

    memset(new_tracer, 0, sizeof(tracer));

    BUG_ON(tracer_type != TRACER_TYPE_INS_VT && tracer_type != TRACER_TYPE_APP_VT);

    InitializeTracerEntry(new_tracer, tracer_id, tracer_type);

    return new_tracer;

}

void FreeTracerEntry(tracer * tracer_entry) {


    llist_destroy(&tracer_entry->schedule_queue);
        llist_destroy(&tracer_entry->run_queue);
    kfree(tracer_entry);

}


/***
Set the time dilation variables to be consistent with all children
***/
void SetChildrenCpu(struct task_struct *aTask, int cpu) {
    struct list_head *list;
    struct task_struct *taskRecurse;
    struct task_struct *me;
    struct task_struct *t;

    if (aTask == NULL) {
        PDEBUG_E("Set Children CPU: Task does not exist\n");
        return;
    }
    if (aTask->pid == 0) {
        return;
    }

    me = aTask;
    t = me;

    /* set policy for all threads as well */
    do {
        if (t->pid != aTask->pid) {
            if (cpu == -1) {

                /* allow all cpus */
                cpumask_setall(&t->cpus_allowed);
            } else {
                bitmap_zero((&t->cpus_allowed)->bits, 8);
                cpumask_set_cpu(cpu, &t->cpus_allowed);
            }
        }
    } while_each_thread(me, t);

    list_for_each(list, &aTask->children) {
        taskRecurse = list_entry(list, struct task_struct, sibling);
        if (taskRecurse->pid == 0) {
            return;
        }


        if (cpu == -1) {
            /* allow all cpus */
            cpumask_setall(&taskRecurse->cpus_allowed);
        } else {
            bitmap_zero((&taskRecurse->cpus_allowed)->bits, 8);
            cpumask_set_cpu(cpu, &taskRecurse->cpus_allowed);
        }
        SetChildrenCpu(taskRecurse, cpu);
    }
}


/***
Set the time variables to be consistent with all children.
Assumes tracer write lock is acquired prior to call
***/
void SetChildrenTime(tracer * tracer_entry,
                       struct task_struct *aTask, s64 time,
                       int increment) {

    struct list_head *list;
    struct task_struct *taskRecurse;
    struct task_struct *me;
    struct task_struct *t;
    unsigned long flags;

    if (aTask == NULL) {
        PDEBUG_E("Set Children Time: Task does not exist\n");
        return;
    }
    if (aTask->pid == 0) {
        return;
    }
    me = aTask;
    t = me;


    // do not set for any threads of tracer itself
    if (tracer_entry->tracer_type == TRACER_TYPE_APP_VT || aTask->pid != tracer_entry->tracer_task->pid) {
        do {
            /* set it for all threads */
            if (t->pid != aTask->pid && t->pid != tracer_entry->tracer_task->pid) {
                        t->virt_start_time = virt_exp_start_time;
                if (increment) {	
                    t->curr_virt_time += time;
                } else {
                    t->curr_virt_time = time;
                }
                if (experiment_status != RUNNING)
                    t->wakeup_time = 0;
            }
        } while_each_thread(me, t);
    }

    list_for_each(list, &aTask->children) {
        taskRecurse = list_entry(list, struct task_struct, sibling);
        if (taskRecurse->pid == 0) {
            return;
        }
            taskRecurse->virt_start_time = virt_exp_start_time;
        if (increment) {
            taskRecurse->curr_virt_time += time;
        } else {
            taskRecurse->curr_virt_time = time;
        }
        
        if (experiment_status != RUNNING)
            taskRecurse->wakeup_time = 0;
        SetChildrenTime(tracer_entry, taskRecurse, time, increment);
    }
}

/*
Assumes tracer read lock is acquired prior to call.
*/
void PrintScheduleList(tracer* tracer_entry) {
    int i = 0;
    lxc_schedule_elem * curr;
    if (tracer_entry != NULL) {
        for (i = 0; i < ScheduleListSize(tracer_entry); i++) {
            curr = llist_get(&tracer_entry->schedule_queue, i);
            if (curr != NULL) {
                PDEBUG_V("Schedule List Item No: %d, TRACER PID: %d, "
                         "TRACEE PID: %d, N_insns_curr_round: %d, "
                         "Size OF SCHEDULE QUEUE: %d\n", i,
                         tracer_entry->tracer_task->pid, curr->pid,
                         curr->quanta_curr_round,
                         ScheduleListSize(tracer_entry));
            }
        }
    }
}


/***
My implementation of the kill system call. Will send a signal to a container.
Used for freezing/unfreezing containers
***/
int KillProcess(struct task_struct *killTask, int sig) {
    struct siginfo info;
    int returnVal;
    info.si_signo = sig;
    info.si_errno = 0;
    info.si_code = SI_USER;
    if (killTask) {
        if ((returnVal = send_sig_info(sig, &info, killTask)) != 0) {
            PDEBUG_E("Kill: Error sending kill msg for pid %d\n",
                     killTask->pid);
        }
    }
    return returnVal;
}


tracer * GetTracerForTask(struct task_struct * aTask) {

    int i = 0, tracer_id;
    tracer * curr_tracer;
    if (!aTask)
        return NULL;

    if (experiment_status != RUNNING)
        return NULL;

    tracer_id = aTask->associated_tracer_id;

    if (!tracer_id)
        return NULL;
    
    return hmap_get_abs(&get_tracer_by_id, tracer_id);
}


void GetTracerStructRead(tracer* tracer_entry) {
    if (tracer_entry) {
        read_lock(&tracer_entry->tracer_lock);
    }
}

void PutTracerStructRead(tracer* tracer_entry) {
    if (tracer_entry) {
        read_unlock(&tracer_entry->tracer_lock);
    }
}

void GetTracerStructWrite(tracer* tracer_entry) {
    if (tracer_entry) {
        write_lock(&tracer_entry->tracer_lock);
    }
}

void PutTracerStructWrite(tracer* tracer_entry) {
    if (tracer_entry) {
        write_unlock(&tracer_entry->tracer_lock);
    }
}

int ConvertStringToArray(char * str, int * arr, int arr_size) 
{ 
    // get length of string str 
    int str_length = strlen(str); 
    int i = 0, j = 0, is_neg = 0;
    memset(arr, 0, sizeof(int)*arr_size);

    if (str[0] != '\0' && str[0] == '-')
        is_neg = 1;
  
    // Traverse the string 
    while (str[i] != '\0') { 

        if (str[i] == '-') {
            i++;
            continue;
        }
  
        // if str[i] is ',' then split 
        if (str[i] == ',') {
            if (is_neg)
               arr[j] = -1*arr[j];

            // Increment j to point to next 
            // array location 
            j++;
            if (str[i+1] != '\0' && str[i+1] == '-')
               is_neg = 1;
            else
               is_neg = 0;

        } 
        else if (j < arr_size) { 
  
            // subtract str[i] by 48 to convert it to int 
            // Generate number by multiplying 10 and adding 
            // (int)(str[i]) 
            arr[j] = arr[j] * 10 + (str[i] - 48); 
        }
	i++;
    }
    if (is_neg)
        arr[j] = -1*arr[j];
    return j + 1;
} 
