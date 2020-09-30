#ifndef __VT_FUNCTIONS
#define __VT_FUNCTIONS
#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "kronos_utility_functions.h"


typedef unsigned long u32;

/**** GENERAL API *****/

//! Returns the current virtual time of a tracer with id = 1
s64 getCurrentVirtualTime(void);

//! Returns the current virtual time of a tracer with specified id
s64 getCurrentTimeTracer(int tracer);


//! Initializes a EXP_CBE experiment with specified number of expected tracers
int initializeExp(int num_expected_tracers);

//! Synchronizes and Freezes all started tracers
int synchronizeAndFreeze(void);

//! Initiates Stoppage of the experiment
int stopExp(void);

//! Only to be used for EXP_CBE type experiment. Instructs the experiment to
//  be run for the specific number of rounds where each round signals advancement
//  of virtual time by the specified duration in nanoseconds.
int progressBy(s64 duration, int num_rounds);

//! Associates a network interface with a tracer. Set tracer_id to 0 to attach to any
//  tracer that spawned a spinner task.
int setNetDeviceOwner(int tracer_id, char* intf_name);

/**** GENERAL API *****/




/**** THESE FUNCTIONS ARE FOR INTERNAL USE BY TRACER BINARIES *****/

//! Registers the calling process as a new tracer
/*!
    \param tracer_id ID of the tracer. Maybe specified as -1 for auto assignment
    \param tracer_type INS-VT or APP-VT
    \param registration_type Specifies if an optional PID is present and
        if the optional process is a spinner
    \param optional_pid PID of any additional task which needs to be grouped
        with this tracer
    \param optional_timeline_id If exp_type is EXP_CS, this parameter specified
        the timeline associated with this tracer.
*/
s64 registerTracer(int tracer_id, int tracer_type, int registration_type,
                   int optional_pid, int optional_timeline_id);

//! If the invoking process is a tracer or associated with a tracer, this returns the
//  assigned ID to that tracer.
int getAssignedTracerID();

//! Increments a tracer's virtual clock
/*!
    \param tracer_id ID of the tracer
    \param increment The clock is incremented by this value
*/
int updateTracerClock(int tracer_id, s64 increment);

//! Invoked by a tracer to signal to the virtual time module about processes which
//  should no longer be managed.
/*!
    \param tracer_id ID of the tracer
    \param pids Refers to list of process-ids which need to be ignored
    \param num_pids Number of process-ids
*/
s64 writeTracerResults(int tracer_id, int* pids, int num_pids);

//! Adds a list of pids to a tracer's schedule queue
/*!
    \param tracer_id ID of the tracer
    \param pids Refers to list of process-ids which need to be added
    \param num_pids Number of process-ids
*/
int addProcessesToTracerSq(int tracer_id, int* pids, int num_pids);


//! Invoked by the tracer to wait until completion of the virtual time
//  experiment
int waitForExit(int tracer_id);


/**** THESE FUNCTIONS ARE FOR INTERNAL USE BY TRACER BINARIES *****/


/******** THESE FUNCTIONS ARE USED IN A EXP-CS EXPERIMENT CONTROLLED BY S3FNET SIMULATOR ***********/

//! Initializes a virtual time managed experiment
/*!
    \param exp_type EXP_CS or EXP_CBE
    \param num_timelines Number of timelines (in case of EXP_CS). Set to 0 for EXP_CBE 
    \param num_expected_tracers Number of tracers which would be spawned
*/
int initializeVtExp(int exp_type, int num_timelines,
                    int num_expected_tracers);


//! Advance a EXP_CS timeline by the specified duration. May be used by a network
//  simulator like S3FNet using composite synchronization mechanism
int progressTimelineBy(int timeline_id, s64 duration);

/******** THESE FUNCTIONS ARE USED IN A EXP-CS EXPERIMENT CONTROLLED BY S3FNET SIMULATOR ***********/


#endif
