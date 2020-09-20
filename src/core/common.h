#ifndef __COMMON_H
#define __COMMON_H

#include "includes.h"
#include "vt_module.h"


//! This file defines some commonly used functions for virtual time management

// ! Returns task with pid in the namespace corresponding to tsk
/*!
    \param pid pid of to search for
    \param tsk Search in the namespace of this task
*/
struct task_struct* GetTaskNs(pid_t pid, struct task_struct * parent);


//! Removes the first entry in a tracer's schedule queue
/*! \param tracer_entry Represents the relevant tracer */
int PopScheduleList(tracer * tracer_entry);

//!  Removes the first entry from a tracer's schedule queue and appends it to the end
/*! \param tracer_entry Represents the relevant tracer */
void RequeueScheduleList(tracer * tracer_entry);

//! Removes the first entry in a tracer's run queue and returns it
/*! \param tracer_entry Represents the relevant tracer */
lxc_schedule_elem * PopRunQueue(tracer * tracer_entry);

//! Removes the first entry from a tracer's run queue and appends it to the end
/*! \param tracer_entry Represents the relevant tracer */
void RequeueRunQueue(tracer * tracer_entry);

//! Removes all entries in a tracer's schedule list
/*! \param tracer_entry Represents the relevant tracer */
void CleanUpScheduleList(tracer * tracer_entry);

//! Removes all entries in a tracer's run queue
/*! \param tracer_entry Represents the relevant tracer */
void CleanUpRunQueue(tracer * tracer_entry);

//! Returns the size of a tracer's schedule queue
/*! \param tracer_entry Represents the relevant tracer */
int ScheduleListSize(tracer * tracer_entry);

//! Returns the size of a tracer's run queue
/*! \param tracer_entry Represents the relevant tracer */
int RunQueueSize(tracer * tracer_entry);

//! Adds a task to a tracer's schedule queue to make it available for scheduling
/*!
    \param tracer_entry Represents the relevant tracer
    \param tracee Represens a new task to be added to a tracer's schedule queue
*/
void AddToTracerScheduleQueue(tracer * tracer_entry,
                              int tracee_pid) ;

//! Removes a  specific tracee from a tracer's schedule queue
/*!
    \param tracer_entry Represents the relevant tracer
    \param tracee_pid Represents the pid of the tracee to remove
*/
void RemoveFromTracerScheduleQueue(tracer * tracer_entry, int tracee_pid);

//! Registers the process which makes this call as a new tracer
/*!
    \param write_buffer The process specifies some parameters in a string buffer
*/
int RegisterTracerProcess(char * write_buffer);

//! Increments the current virtual time of all tracees added to a tracer's schedule queue
/*!
    \param tracer_entry Represents the relevant tracer
    \param time_increment Represents amount of increment in virtual time
*/
void UpdateAllChildrenVirtualTime(tracer * tracer_entry, s64 time_increment);


void UpdateInitTaskVirtualTime(s64 time_to_set);

//! Updates the virtual time of all tracers aligned on a given timeline
/*! \param timelineID The ID of the relvant timeline to look into */
void UpdateAllTracersVirtualTime(int timelineID);

//! Invoked by a tracee to signal completion of its run
// The tracee may signal whether it wishes to be ignored/removed from a tracer's
// schedule queue as well
/*!
    \param curr_tracer Points to the tracer which is responsibe for the tracee
        making this call
    \param api_args Represents a list of pids which must be removed from a tracer's
        virtual schedule queue. A tracee may specify its own pid here as well
    \param num_args Represents the number of pids included in api_args
*/
int HandleTracerResults(tracer * curr_tracer, int * api_args, int num_args);

//! Handles stopping of the virtual time controlled experiment
int HandleStopExpCmd();


//! Handles experiment initialization
int HandleInitializeExpCmd(char * buffer);

//! Associates a Networkdevice under Kronos control.
int HandleSetNetdeviceOwnerCmd(char * write_buffer);

//! Returns the current virtual time of the task in question
s64 GetDilatedTime(struct task_struct * task);

//! Returns the current virtual time of a pid specified in a string buffer
/*!
    \param write_buffer A string buffer which contains the interested process's
        pid
*/
s64 HandleGettimePID(char * write_buffer);

//! Blocks the caller process until a tracee executes its current run
/*!
    \param curr_tracer Represents the INS-VT tracer which is supposed to run next
*/
void WaitForInsVTTracerCompletion(tracer * curr_tracer);

//! Blocks the caller process until a tracee executes its current run
/*!
    \param curr_tracer Represents the tracer whose tracee is currently running
    \param relevant_task Represents the interested tracee which is currently
        running
*/
void WaitForAppVTTracerCompletion(tracer * curr_tracer,
                                  struct task_struct * relevant_task);

//! Signals a timeline thread/cpu worker responsible for a tracer to resume execution
/*! \param curr_tracer Represents the relvant tracer is question */
void SignalCPUWorkerResume(tracer * curr_tracer);

#endif