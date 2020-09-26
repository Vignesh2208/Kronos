#include "tracer.h"
#include <sys/sysinfo.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/file.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <sched.h>
#include <getopt.h>
#include <string.h>
#include <sched.h>
#include <signal.h>


#define MAX_BUF_SIZ 1000

struct sched_param param;

//! A custom comparison of two tracee's to check if they are equal based on their PIDs
int llist_elem_comparer(tracee_entry * elem1, tracee_entry * elem2) {

    if (elem1->pid == elem2->pid)
        return 0;
    return 1;

}

//! Prints a statement possibly to stdout or to a log file
void printLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    fflush(stdout);
    va_end(args);
}

//! Allocates a new tracee with specified pid
tracee_entry * allocNewTracee(pid_t pid) {

    tracee_entry * tracee;


    tracee = (tracee_entry *) malloc(sizeof(tracee_entry));
    if (!tracee) {
        LOG("MALLOC ERROR !. Exiting for now\n");
        exit(FAIL);
    }

    tracee->pid = pid;
    tracee->vfork_stop = 0;
    tracee->syscall_blocked = 0;
    tracee->vfork_parent = NULL;
    tracee->n_insns_share = 0;
    tracee->n_insns_left = 0;
    tracee->vrun_time = 0;
    tracee->n_preempts = 0;
    tracee->n_sleeps = 0;
    tracee->pd = NULL;
    tracee->added = 0;

    return tracee;

}

//! Waits for all tracees to startup and sets ptrace options.
void setupAllTracees(hashmap * tracees, llist * tracee_list, llist * run_queue) {

    llist_elem * head = tracee_list->head;
    tracee_entry * tracee;
    int ret = 0;
    unsigned long status = 0;

    while (head != NULL) {
        tracee = (tracee_entry *) head->item;
        do {
            ret = waitpid(tracee->pid, &status, 0);
        } while ((ret == (pid_t) - 1 && errno == EINTR ));

        if (errno == ESRCH) {
            LOG("Child %d is dead. Removing from active tids ...\n", tracee->pid);
            tracee->added = -1;

        } else {
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
                LOG("Child %d stopped. Setting trace options \n", tracee->pid);
                ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL, PTRACE_O_EXITKILL |
                       PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXIT |
                       PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK |
                       PTRACE_O_TRACEEXEC);

                //ptrace(PTRACE_DETACH, tracee->pid, 0, SIGSTOP);
                if (errno == ESRCH)
                    LOG("Error setting ptrace options\n");
            }
        }

        head = head->next;

    }

}

//! Waits for a previously initiated INS-SCHED operation to complete
/* Input:
 *		tracees: Hashmap of <pid, tracee_entry>
 *		tracees_list: Linked list of <tracee_entry>
 *      run_queue: List of tracees which are not blocked
 *		pid:	PID of the tracee which is to be run
 *		pd: Associated libperf counter for the tracee
 *		cpu_assigned: Cpu on which the tracer and its tracees are running
 * Output:
 *		STATUS of the process upon return.
 */
int waitForPtraceEvents(hashmap * tracees, llist * tracee_list,
                        llist * run_queue, pid_t pid,
                        struct libperf_data * pd, int cpu_assigned) {

    llist_elem * head = tracee_list->head;
    tracee_entry * new_tracee;
    tracee_entry * curr_tracee;

    struct user_regs_struct regs;

    int ret;
    u32 n_ints;
    u32 status = 0;
    uint64_t counter;


    pid_t new_child_pid;
    struct libperf_data * pd_tmp;
    u32 n_insns = 1000;


    curr_tracee = hmap_get_abs(tracees, pid);
    if (curr_tracee->vfork_stop)	//currently inside a vfork stop
        return TID_FORK_EVT;

retry:
    status = 0;
    errno = 0;
    do {
        ret = waitpid(pid, &status, WTRACE_DESCENDENTS | __WALL);
    } while (ret == (pid_t) - 1 && errno == EINTR);

    if ((pid_t)ret != pid) {
        if (errno == EBREAK_SYSCALL) {
            LOG("Waitpid: Breaking out. Process entered blocking syscall\n");
            curr_tracee->syscall_blocked = 1;
            return TID_SYSCALL_ENTER;
        }
                printf("Waitpid ERROR. Ignoring process\n");
                fflush(stdout);
        return TID_IGNORE_PROCESS;
    }



    if (errno == ESRCH) {
        printf("Process does not exist\n");
                fflush(stdout);
        return TID_IGNORE_PROCESS;
    }

    LOG("Waitpid finished for Process : %d. Status = %lX\n ", pid, status);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {


        // check for different types of events and determine whether
        // this is a system call stop detected fork or clone event. get new tid.
        if (status >> 8 == (SIGTRAP | PTRACE_EVENT_CLONE << 8)
                || status >> 8 == (SIGTRAP | PTRACE_EVENT_FORK << 8)) {
            LOG("Detected PTRACE EVENT CLONE or FORK for : %d\n", pid);
            ret = ptrace(PTRACE_GETEVENTMSG, pid, NULL, (u32*)&new_child_pid);
            status = 0;
            do {
                ret = waitpid(new_child_pid, &status, __WALL);
            } while (ret == (pid_t) - 1 && errno == EINTR);

            if (errno == ESRCH) {
                printf("Child Process does not exist\n");
                                fflush(stdout);
                return TID_IGNORE_PROCESS;
            }
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
                new_tracee = allocNewTracee(new_child_pid);
                llist_append(tracee_list, new_tracee);
                llist_append(run_queue, new_tracee);
                hmap_put_abs(tracees, new_child_pid, new_tracee);
                LOG("Detected new cloned child with tid: %d. status = %lX, "
                    "Ret = %d, errno = %d, Set trace options.\n", new_child_pid,
                    status, ret, errno);
                new_tracee->pd = libperf_initialize((int)new_child_pid, cpu_assigned);
                return TID_FORK_EVT;
            } else {
                printf("ERROR ATTACHING TO New Thread "
                       "status = %lX, ret = %d, errno = %d\n", status, ret, errno);
                                fflush(stdout);
                return TID_IGNORE_PROCESS;
            }
        } else if (status >> 8 == (SIGTRAP | PTRACE_EVENT_EXIT << 8)) {
            LOG("Detected process exit for : %d\n", pid);
            if (curr_tracee->vfork_parent != NULL) {
                curr_tracee->vfork_parent->vfork_stop = 0;
                curr_tracee->vfork_parent = NULL;
            }
            return TID_EXITED;
        } else if (status >> 8 == (SIGTRAP | PTRACE_EVENT_VFORK << 8)) {
            LOG("Detected PTRACE EVENT VFORK for : %d\n", pid);
            ret = ptrace(PTRACE_GETEVENTMSG, pid, NULL, (u32*)&new_child_pid);
            status = 0;
            do {
                ret = waitpid(new_child_pid, &status, __WALL);
            } while (ret == (pid_t) - 1 && errno == EINTR);

            if (errno == ESRCH) {
                printf("ERROR Ptrace VFORK handling.\n");
                                fflush(stdout);
                return TID_IGNORE_PROCESS;
            }
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
                new_tracee = allocNewTracee(new_child_pid);
                llist_append(tracee_list, new_tracee);
                llist_append(run_queue, new_tracee);
                hmap_put_abs(tracees, new_child_pid, new_tracee);
                LOG("Detected new vforked process with pid: %d. "
                    "status = %lX, Ret = %d, errno = %d, Set trace options.\n",
                    new_child_pid, status, ret, errno);
                curr_tracee->vfork_stop = 1;
                new_tracee->vfork_parent = curr_tracee;
                new_tracee->pd = libperf_initialize((int)new_child_pid, cpu_assigned);
                return TID_FORK_EVT;
            } else {
                printf("ERROR ATTACHING TO New Thread "
                       "status = %lX, ret = %d, errno = %d\n", status, ret, errno);
                return TID_IGNORE_PROCESS;
            }

        } else if (status >> 8 == (SIGTRAP | PTRACE_EVENT_EXEC << 8)) {
            LOG("Detected PTRACE EVENT EXEC for : %d\n", pid);
            if (curr_tracee->vfork_parent != NULL) {
                curr_tracee->vfork_parent->vfork_stop = 0;
                curr_tracee->vfork_parent = NULL;
            }

            return TID_OTHER;
        } else {


            ret = ptrace(PTRACE_GET_REM_MULTISTEP, pid, 0, (u32*)&n_ints);
            ret = ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            LOG("Single step completed for Process : %d. "
                "Status = %lX. Rip: %lX\n\n ", pid, status, regs.rip);
            if (ret == -1) {
                LOG("ERROR in GETREGS.\n");
            }
            return TID_SINGLESTEP_COMPLETE;

        }


    } else {
        if (WIFSTOPPED(status) && WSTOPSIG(status) >= SIGUSR1
                && WSTOPSIG(status) <= SIGALRM ) {
            LOG("Received Signal: %d\n", WSTOPSIG(status));
            errno = 0;
            n_insns = 1000;
            // init lib
            libperf_ioctlrefresh(pd, LIBPERF_COUNT_HW_INSTRUCTIONS,
                                (uint64_t )n_insns);

            // enable hardware counter
            libperf_enablecounter(pd, LIBPERF_COUNT_HW_INSTRUCTIONS);
            ret = ptrace(PTRACE_SET_REM_MULTISTEP, pid, 0, (u32*)&n_insns);

            LOG("PTRACE RESUMING process After signal. "
                "ret = %d, error_code = %d. pid = %d\n", ret, errno, pid);
            fflush(stdout);
            ret = ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status));
            goto retry;


        } else if (WIFSTOPPED(status) && WSTOPSIG(status) != SIGCHLD) {
            LOG("Received Exit Signal: %d\n", WSTOPSIG(status));
            if (curr_tracee->vfork_parent != NULL) {
                curr_tracee->vfork_parent->vfork_stop = 0;
                curr_tracee->vfork_parent = NULL;
            }
            ret = ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status));
            return TID_EXITED;
        } else {
            LOG("Received Unknown Signal: %d\n", WSTOPSIG(status));
        }
    }


    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGCHLD) {
        LOG("Received SIGCHLD. Process exiting...\n");
        if (curr_tracee->vfork_parent != NULL) {
            curr_tracee->vfork_parent->vfork_stop = 0;
            curr_tracee->vfork_parent = NULL;
        }
        return TID_EXITED;
    } else {
        LOG("Received Unknown Status: %d\n", WSTOPSIG(status));
    }
    return TID_OTHER;
}

//! Checks if a tracee is currently blocked waiting for an I/O operation to complete
int isTraceeBlocked(tracee_entry * curr_tracee) {
    u32 flags = 0;
    u32 status = 0;
    int ret;
    pid_t pid;

    if (!curr_tracee)
        return TID_NOT_MONITORED;

    pid = curr_tracee->pid;
    if (curr_tracee->syscall_blocked) {
        errno = 0;
        ret = ptrace(PTRACE_GET_MSTEP_FLAGS,
                     pid, 0, (u32*)&flags);

        if (flags == 0) {
            LOG("Waiting for unblocked tracee: %d to reach stop state\n",
                curr_tracee->pid); 
            errno = 0;
            do {
                ret = waitpid(pid, &status,
                              WTRACE_DESCENDENTS | __WALL);
            } while (ret == (pid_t) - 1 && errno == EINTR);

            if ((pid_t)ret != pid) {
                LOG("Error during wait\n");
                return TID_PROCESS_BLOCKED;
            }
            LOG("Tracee: %d unblocked\n", curr_tracee->pid);
            
            curr_tracee->syscall_blocked = 0;
            return FAIL;
        } else{
            LOG("Tracee : %d Still Blocked Flags: %d\n", pid, flags);
            return TID_PROCESS_BLOCKED;
        }
    }
    if (curr_tracee->vfork_stop) {
        return TID_PROCESS_BLOCKED;
    }
    return FAIL;
}


/*
 * Runs a tracee specified by its pid for a specific number of instructions
 * and returns the result of the operation.
 * Input:
 *		tracees: Hashmap of <pid, tracee_entry>
 *		tracees_list: Linked list of <tracee_entry>
 *		pid:	PID of the tracee which is to be run
 *		n_insns: Number of instructions by which the specified tracee is advanced
 *		cpu_assigned: Cpu on which the tracer is running
 *		rel_cpu_speed: Kronos specific relative cpu speed assigned to the
         tracer. Equivalent to TDF.
 * Output:
 *		SUCCESS or FAIL
 */
int runCommandedProcess(hashmap * tracees, llist * tracee_list,
                        llist * run_queue, pid_t pid,
                        u32 n_insns_orig, int cpu_assigned, float rel_cpu_speed) {


    int ret;
    int singlestepmode = 1;
    tracee_entry * curr_tracee;
    unsigned long buffer_window_length = BUFFER_WINDOW_SIZE;
    u32 n_insns;


    curr_tracee = hmap_get_abs(tracees, pid);
    if (!curr_tracee)
        return FAIL;

    n_insns = (u32) (n_insns_orig * rel_cpu_speed);

    LOG("Running Child: %d Quanta: %d instructions\n", pid, n_insns);

    if (n_insns <= buffer_window_length)
        singlestepmode = 1;
    else
        singlestepmode = 0;

    while (1) {
        errno = 0;
        if (isTraceeBlocked(curr_tracee) == TID_PROCESS_BLOCKED) {
            LOG("Unblocked tracee %d blocked again\n", curr_tracee->pid);
            return SUCCESS;
        }

        if (singlestepmode) {
            
            ret = ptrace(PTRACE_SET_REM_MULTISTEP, pid, 0, (u32*)&n_insns);
            LOG("PTRACE RESUMING MULTI-STEPPING OF process. "
                "ret = %d, error_code = %d, pid = %d, n_insns = %d\n", ret,
                errno, pid, n_insns);

            ret = ptrace(PTRACE_MULTISTEP, pid, 0, (u32*)&n_insns);

        } else {

            n_insns = n_insns - buffer_window_length;
            ptrace(PTRACE_SET_DELTA_BUFFER_WINDOW, pid, 0,
                   (unsigned long *)&buffer_window_length);
            // init lib
            if (curr_tracee->pd == NULL) 
                curr_tracee->pd = libperf_initialize((int)pid, cpu_assigned);

            libperf_ioctlrefresh(curr_tracee->pd, LIBPERF_COUNT_HW_INSTRUCTIONS,
                                 (uint64_t )n_insns);
            // enable HW counter
            libperf_enablecounter(curr_tracee->pd, LIBPERF_COUNT_HW_INSTRUCTIONS);
            ret = ptrace(PTRACE_SET_REM_MULTISTEP, pid, 0, (u32*)&n_insns);			
            ret = ptrace(PTRACE_CONT, pid, 0, 0);
            LOG("PTRACE RESUMING process. "
                "ret = %d, error_code = %d. pid = %d, n_insns = %d\n",
                ret, errno, pid, n_insns);

        }

        if (ret < 0 || errno == ESRCH) {
            usleep(100000);
            if (kill(pid, 0) == -1 && errno == ESRCH) {
                LOG("PTRACE_RESUME ERROR. Child process is Dead. Breaking\n");
                if (curr_tracee->vfork_parent != NULL) {
                    curr_tracee->vfork_parent->vfork_stop = 0;
                    curr_tracee->vfork_parent = NULL;
                }
                if (curr_tracee->pd != NULL) {
                    libperf_disablecounter(curr_tracee->pd,
                            LIBPERF_COUNT_HW_INSTRUCTIONS);
                    libperf_finalize(curr_tracee->pd, 0);
                }
                curr_tracee->added = -1;
                return FAIL;
            }
            printf("PTRACE CONT Error: %d\n", errno);
                        fflush(stdout);
            n_insns = (u32) (n_insns_orig * rel_cpu_speed);
            continue;
        }


        ret = waitForPtraceEvents(tracees, tracee_list, run_queue, pid, curr_tracee->pd,
                                     cpu_assigned);
        switch (ret) {

        case TID_SYSCALL_ENTER:
            curr_tracee->syscall_blocked = 1;
            curr_tracee->n_preempts ++;
            return SUCCESS;

        case TID_IGNORE_PROCESS:
            curr_tracee->added = -1;
            // For now, we handle this case like this.
            ptrace(PTRACE_CONT, pid, 0, 0);
            usleep(10);
            printf("Process: %d, IGNORED\n", pid);
                        fflush(stdout);
            if (curr_tracee->pd != NULL)
                libperf_finalize(curr_tracee->pd, 0);
            return TID_IGNORE_PROCESS;


        case TID_EXITED:
            curr_tracee->added = -1;
            // Exit is still not fully complete. need to do this to complete it.
            ret = ptrace(PTRACE_DETACH, pid, 0, SIGCHLD);
            LOG("Ptrace Detach: ret: %d, errno: %d\n", ret, errno);
            LOG("Process: %d, EXITED\n", pid);
            if (curr_tracee->pd != NULL)
                libperf_finalize(curr_tracee->pd, 0);
            return TID_IGNORE_PROCESS;

        default:		return SUCCESS;

        }
        break;
    }
    return FAIL;


}

//! In a given round for a specified total number of instructions, it runs
//  all tracees in a round robin fashion with each tracee's quanta computed
//  fairly.
void runProcessesInRound(int tracer_id, hashmap * tracees, llist * tracee_list,
                        u32 n_round_insns, llist * run_queue,
                        llist * blocked_tracees, int cpu_assigned,
                        float rel_cpu_speed) {

    int base_insn_share = SMALLEST_PROCESS_QUANTA_INSNS;
    int status;
    u32 n_insns_run = 0;
    unsigned long arg;
    int n_checked_processes;
    int n_blocked_processes;
    tracee_entry * tracee;
    u32 n_insns_to_run;

    do {

        n_insns_to_run = 0;
        n_checked_processes = 0;
        n_blocked_processes = llist_size(blocked_tracees);

        while (n_checked_processes < n_blocked_processes) {
            tracee_entry * curr_tracee = llist_get(blocked_tracees, 0);
            if (!curr_tracee)
                break;

            if (curr_tracee->added < 0) {
                n_checked_processes ++;
                llist_remove(blocked_tracees, curr_tracee);
                continue;
            }

            if (isTraceeBlocked(curr_tracee) == FAIL) {
                llist_pop(blocked_tracees);
                llist_append(run_queue, curr_tracee);

            } else {
                curr_tracee->n_sleeps ++;
                if (n_blocked_processes > 1)
                    llist_requeue(blocked_tracees);
            }
            n_checked_processes ++;
        }


        while(1) {
            tracee = llist_get(run_queue, 0);
            if (tracee && tracee->added < 0) {
                llist_remove(run_queue, tracee);
                continue;
            }
            break;
        }		

        if (!tracee) {

            if ((n_round_insns - n_insns_run) > base_insn_share) {
                n_insns_run += base_insn_share;
                n_insns_to_run = base_insn_share;
            } else {
                n_insns_to_run = n_round_insns - n_insns_run;
                n_insns_run = n_round_insns;

            }
            //usleep((n_insns_to_run * rel_cpu_speed) / 1000);
            LOG("No Runnable tracees. !\n");
            if (n_insns_run < n_round_insns)
                continue;
            else
                return;
        }

        if (tracee->n_insns_share == 0)
            tracee->n_insns_share = base_insn_share;

        if (tracee->n_insns_left) {

            if (n_insns_run + tracee->n_insns_left > n_round_insns) {
                n_insns_to_run = n_round_insns - n_insns_run;
                n_insns_run = n_round_insns;
                tracee->n_insns_left -= n_insns_to_run;
            } else {
                n_insns_run += tracee->n_insns_left;
                n_insns_to_run = tracee->n_insns_left;
                tracee->n_insns_left = 0;
            }
        } else {
            if (n_insns_run + tracee->n_insns_share > n_round_insns) {
                n_insns_to_run = n_round_insns - n_insns_run;
                n_insns_run = n_round_insns;
                tracee->n_insns_left = tracee->n_insns_share - n_insns_to_run;
            } else {
                n_insns_run += tracee->n_insns_share;
                n_insns_to_run = tracee->n_insns_share;
                tracee->n_insns_left = 0;
            }

        }

        assert(n_insns_to_run > 0);

        LOG("Running %d for %d\n instructions\n", tracee->pid, n_insns_to_run);
        status =
            runCommandedProcess(tracees, tracee_list, run_queue,
                                  tracee->pid, n_insns_to_run,
                                  cpu_assigned, rel_cpu_speed);
        LOG("Ran %d for %d\n instructions\n", tracee->pid, n_insns_to_run);


        if (n_insns_to_run > 0 && n_insns_run < n_round_insns)
            updateTracerClock(tracer_id, (s64)n_insns_to_run);

        tracee->vrun_time += (n_insns_to_run / 1000);

        if (status == SUCCESS) {

            if (tracee->syscall_blocked) {
                // tracee is blocked after running
                tracee->n_insns_left = 0;
                llist_pop(run_queue);
                llist_append(blocked_tracees, tracee);
            } else {
                if (tracee->n_insns_left == 0) {
                    llist_requeue(run_queue);
                } else {
                    // process not blocked and n_insns_left > 0. we reached end.
                    LOG("N-insns left %d Pid: %d\n", tracee->n_insns_left, tracee->pid);
                    break;
                }
            }
        }



    } while (n_insns_run < n_round_insns);
}


void printUsage(int argc, char* argv[]) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [ -h | --help ]\n", argv[0]);
    fprintf(stderr,	"		%s -i TRACER_ID [-t TimelineID]"
            "[-f CMDS_FILE_PATH or -c \"CMD with args\"] -r RELATIVE_CPU_SPEED "
            "-s CREATE_SPINNER\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "This program executes all COMMANDs specified in the "
            "CMD file path or the specified CMD with arguments in trace mode "
            "under the control of Kronos\n");
    fprintf(stderr, "\n");
}


int main(int argc, char * argv[]) {

    char * cmd_file_path = NULL;
    char * line;
    size_t line_read;
    size_t len = 0;

    llist tracee_list;
    llist run_queue;
    llist blocked_tracees;
    hashmap tracees;
    tracee_entry * new_entry;

    pid_t new_cmd_pid;
    int tracer_id = -1; // set to -1 by default for automatic allotment
    float rel_cpu_speed = 1.0;
    u32 n_round_insns;
    int cpu_assigned;
    char command[MAX_BUF_SIZ];
    int exited_pids[MAX_CHANGED_PROCESSES_PER_ROUND];
    int created_pids[MAX_CHANGED_PROCESSES_PER_ROUND];
    int create_spinner = 0;
    pid_t spinned_pid;
    FILE* fp;
    int option = 0;
    int read_from_file = -1;
    int timeline_id=0;
    int i, n_tracees, num_created, num_exited, round_no;

    hmap_init(&tracees, 1000);
    llist_init(&tracee_list);
    llist_init(&run_queue);
    llist_init(&blocked_tracees);

    llist_set_equality_checker(&tracee_list, llist_elem_comparer);
    llist_set_equality_checker(&run_queue, llist_elem_comparer);
    llist_set_equality_checker(&blocked_tracees, llist_elem_comparer);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printUsage(argc, argv);
        exit(FAIL);
    }


    while ((option = getopt(argc, argv, "i:f:r:st:c:h")) != -1) {
        switch (option) {
        case 'i' : tracer_id = atoi(optarg);
            if (tracer_id <= 0) { 
                fprintf(stderr, "Explicitly specified tracer=id must be > 0"); exit(FAIL);
            }
            break;
        case 'r' : rel_cpu_speed = atof(optarg);
            break;
        case 'f' : cmd_file_path = optarg;
            read_from_file = 1;
            break;
        case 's' : create_spinner = 1;
            break;
        case 't' : timeline_id = atoi(optarg);
            break;
        case 'c' : memset(command, 0, sizeof(char)*MAX_BUF_SIZ);
            sprintf(command, "%s\n", optarg);
            read_from_file = 0;
            break;
        case 'h' :
        default: printUsage(argc, argv);
            exit(FAIL);
        }
    }

    if (read_from_file == -1) {
        fprintf(stderr, "Missing command or command file path. Atleast one of them must be specified.\n");
        printUsage(argc, argv);
        exit(FAIL);
    }

    

    if (read_from_file)
        LOG("CMDS_FILE_PATH: %s\n", cmd_file_path);
    else
        LOG("CMD TO RUN: %s\n", command);



    if (read_from_file) {
        fp = fopen(cmd_file_path, "r");
        if (fp == NULL) {
            LOG("ERROR opening cmds file path\n");
            exit(FAIL);
        }
        while ((line_read = getline(&line, &len, fp)) != -1) {
            LOG("Starting Command: %s", line);
            runCommand(line, &new_cmd_pid);
            new_entry = allocNewTracee(new_cmd_pid);
            llist_append(&tracee_list, new_entry);
            llist_append(&run_queue, new_entry);
            hmap_put_abs(&tracees, new_cmd_pid, new_entry);
        }
        fclose(fp);
    } else {
        LOG("Starting Command: %s", command);
        runCommand(command, &new_cmd_pid);
        new_entry = allocNewTracee(new_cmd_pid);
        llist_append(&tracee_list, new_entry);
        llist_append(&run_queue, new_entry);
        hmap_put_abs(&tracees, new_cmd_pid, new_entry);
    }

    setupAllTracees(&tracees, &tracee_list, &run_queue);

    if (create_spinner) {
        createSpinnerTask(&spinned_pid);
        cpu_assigned = registerTracer(tracer_id, TRACER_TYPE_INS_VT, REGISTRATION_W_SPINNER,
            spinned_pid, timeline_id);
    } else
        cpu_assigned = registerTracer(tracer_id, TRACER_TYPE_INS_VT, SIMPLE_REGISTRATION, 0,
            timeline_id);

    if (tracer_id == -1) {
       tracer_id = getAssignedTracerID();
       if (tracer_id <= 0) {
           fprintf(stderr, "This shouldn't happen. Auto assigned tracer-id is negative ! Make sure experiment is initialized before starting a tracer.");
           exit(FAIL);
       }
    }

    printf("Tracer PID: %d\n", (pid_t)getpid());
    printf("ASSIGNED Tracer ID: %d\n", tracer_id);
    printf("REL_CPU_SPEED: %f\n", rel_cpu_speed);
    fflush(stdout);

    if (cpu_assigned < 0) {
            LOG("TracerID: %d, Registration Error. Errno: %d\n", tracer_id,
                errno);
            exit(FAIL);
    } else {
        LOG("TracerID: %d, Assigned CPU: %d\n", tracer_id, cpu_assigned);
    }

    round_no = 0;

    while (1) {

        
        memset(exited_pids, 0, sizeof(int)* MAX_CHANGED_PROCESSES_PER_ROUND);
        memset(created_pids, 0, sizeof(int)* MAX_CHANGED_PROCESSES_PER_ROUND);
        round_no ++;

        llist_elem * head = tracee_list.head;
        tracee_entry * tracee;

        num_created = 0, num_exited = 0;
        while (head != NULL) {
            tracee = (tracee_entry *) head->item;
            if (tracee->added == 0) {
                created_pids[num_created++] = tracee->pid;
                tracee->added = 1;
            } else if (tracee->added == -1) {
                exited_pids[num_exited++] = tracee->pid;
            }
            tracee->n_insns_share = SMALLEST_PROCESS_QUANTA_INSNS;
            head = head->next;
        }

        for (i = 0; i < num_exited; i++) {
            tracee = (tracee_entry *) hmap_get_abs(&tracees, exited_pids[i]);
            if (tracee) {
                hmap_remove_abs(&tracees, exited_pids[i]);
                llist_remove(&tracee_list, tracee);
                llist_remove(&run_queue, tracee);
                llist_remove(&blocked_tracees, tracee);
                free(tracee);
            }
        }

        if (num_created) {
            addProcessesToTracerSq(tracer_id, created_pids, num_created);
        }

        printTraceeList(&tracee_list);

        n_round_insns = writeTracerResults(tracer_id, exited_pids, num_exited);
        if (!n_round_insns) {
            LOG("TracerID: %d, STOP Command received. Stopping tracer ...\n",
                tracer_id);
            goto end;
        }

        LOG("TracerID: %d, ################## ROUND: %d, Num round Insns: %d ########################\n",
            tracer_id, round_no, n_round_insns);
        
        runProcessesInRound(tracer_id, &tracees, &tracee_list,
                               n_round_insns, &run_queue,
                       &blocked_tracees, cpu_assigned,
                       rel_cpu_speed);

    }

    end:
    if (create_spinner)
        kill(spinned_pid, SIGKILL);

    n_tracees = llist_size(&tracee_list);
    for (i = 0; i < n_tracees; i++) {
        tracee_entry * curr_tracee = llist_pop(&tracee_list);
        if (!curr_tracee)
            break;
        if (curr_tracee->pd != NULL) {
            libperf_disablecounter(curr_tracee->pd,
                                   LIBPERF_COUNT_HW_INSTRUCTIONS);
            libperf_finalize(curr_tracee->pd, 0);
        }

        hmap_remove_abs(&tracees, curr_tracee->pid);
        llist_remove(&run_queue, curr_tracee);
        free(curr_tracee);
    }
    llist_destroy(&run_queue);
    llist_destroy(&tracee_list);
    hmap_destroy(&tracees);
    fflush(stdout);
    return 0;
}
