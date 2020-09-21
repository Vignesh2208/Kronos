#ifndef __SYNC_EXPERIMENT_H
#define __SYNC_EXPERIMENT_H

#include "includes.h"
#include "vt_module.h"


//! Cleans up all memory allocations associated with a virtual time managed experiment
void CleanExp();

//! Appropriately schedules tracees associated with a tracer
int UnfreezeProcExpRecurse(tracer * curr_tracer);

//! Gets the nex tracee belonging to a tracer which can be run 
lxc_schedule_elem * GetNextRunnableTask(tracer * curr_tracer);

//! For each tracee in a tracer, updates the maximum virtual time duration for which it
//  can be run before yielding the cpu to another tracee. Returns the number
//  of runnable tracees
int UpdateAllRunnableTaskTimeslices(tracer * curr_tracer);

//! Removes any tracees which may have died, from a tracer's schedule queue
void CleanUpAllIrrelevantProcesses(tracer * curr_tracer);


//! Searches all children recursively of a tracer to find a process
/*!
    \param aTask Represents the parent process. Initially set to point to the tracer
        process
    \param pid Process to search for among its children
*/
struct task_struct * SearchTracer(struct task_struct * aTask, int pid);


//! Synchronizes and freezes all tracers to a common virtual time
int SyncAndFreeze();

//! Cleans up/frees all allocated resources
int CleanUpExperimentComponents();

//! Allocates memory and initializes timeline workers and clocks
/*!
    \param exp_type Type of the experiment: EXP_CS or EXP_CBE
    \param num_timelines Number of timelines (in EXP_CS) or number of cpu
        workers (in EXP_CBE)
    \param num_expected_tracers Number of tracers which need to finish
        registration before the experiment can proceed further
*/
int InitializeExperimentComponents(int exp_type, int num_timelines, int num_expected_tracers);


//! Frees memory associated with all tracers
void FreeAllTracers();


//! Stops all timeline workers and initiates experiment stopping.
void InitiateExperimentStopOperation();


//! Invoked for a EXP_CBE experiment to run it for the specified number of rounds
/*!
    \param progress_duration Duration to advance by in each round
    \param num_rounds Number of rounds to run for
*/
int ProgressBy(s64 progress_duration, int num_rounds);

//! Invoked for a EXP_CS experiment to run a specific timeline for the specified duration
/*!
    \param timeline_id Represents the timeline in question
    \param progress_duration Duration to run the timeline for
*/
int ProgressTimelineBy(int timeline_id, s64 progress_duration);

#endif
