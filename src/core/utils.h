#ifndef __VT_UTILS_H
#define __VT_UTILS_H

#include <linux/string.h>
#include "vt_module.h"


//! Allocates pages which are memory mapped and used to hold APP-VT tracer clocks
char * AllocMmapPages(int npages);

//! Frees any allocated pages which hold APP-VT tracer clocks
void FreeMmapPages(void *mem, int npages);

//! Returns the first int value present in the string buffer (until a non)
// numeric character is reached
int GetNextValue (char *write_buffer);


//! Converts a string to int
int atoi(char *s);


//! Returns a task struct for the specified pid.
struct task_struct* FindTaskByPid(unsigned int nr);


//! Initializes a newly created tracer
/*!
    \param new_tracer An uninitialized tracer 
    \param tracer_id Id to assign to the tracer
    \param tracer_type Type of the tracer: INS-VT or APP-VT
        INS-VT tracers are based on INS-SCHED while app-vt tracers are based on
        PIN tools for instruction counting.
*/
void InitializeTracerEntry(tracer * new_tracer, uint32_t tracer_id,
                           int tracer_type);


//! Allots and initializes memory for a new tracer with the specified ID
tracer * AllocTracerEntry(uint32_t tracer_id, int tracer_type);


//! Frees memory associated with the specified tracer
void FreeTracerEntry(tracer * tracer_entry);


//! Constraints the task and all its children to the specified CPU
void SetChildrenCpu(struct task_struct *aTask, int cpu);


//! Sets the virtual time for all tracees of a tracer
/*!
    \param tracer_entry Represents the relevant tracer
    \param time Virtual time to set
    \param increment If set, then "time" will be added to the existing
        virtual clock of each tracee
*/
void SetChildrenTime(tracer * tracer_entry,
                     struct task_struct *aTask, s64 time, int increment);


//! Prints the set of tracees in a tracer's schedule queue 
void PrintScheduleList(tracer* tracer_entry);


//! Sends a signal to the specific process
/*!
    \params killTask Represents the task to send the signal to
    \params sig Represents the signal number
*/
int KillProcess(struct task_struct *killTask, int sig);

//! Returns a registered tracer which owns this task.
tracer * GetTracerForTask(struct task_struct * aTask);


//! Acquires a thread-safe read lock which allows reading details about the relevant tracer
void GetTracerStructRead(tracer* tracer_entry);

//! Releases a thread-safe read lock which prevents reading details about the relevant tracer
void PutTracerStructRead(tracer* tracer_entry);


//! Acquires a thread-safe write lock which allows changing details of the relevant tracer
void GetTracerStructWrite(tracer* tracer_entry);


//! Releases a thread-safe write lock which prevents changing details of the relevant tracer
void PutTracerStructWrite(tracer* tracer_entry);


//! Takes in a string containing comma separated numbers and returns an array of numbers
/*!
    \param str The string to process
    \param arr An integer array of a specific size which has already been allocated
    \param arr_size Max size of the integer array
*/
int ConvertStringToArray(char * str, int * arr, int arr_size);

#endif