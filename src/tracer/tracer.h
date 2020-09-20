#ifndef __TRACER_H
#define __TRACER_H

#include "Kronos_functions.h"
#include "includes.h"
#include "utils.h"


typedef struct tracee_entry_struct {
  pid_t pid;
  int vfork_stop;
  int syscall_blocked;
  u32 n_insns_share;
  int n_insns_left;
  int n_preempts;
  int n_sleeps;
  int added;
  u32 vrun_time;
  struct tracee_entry_struct *vfork_parent;
  struct libperf_data *pd;

} tracee_entry;


#endif
