
#include "Kronos_functions.h"
#include <sys/ioctl.h>
#include "kronos_utility_functions.h"

s64 registerTracer(int tracer_id, int tracer_type, int registration_type,
                   int optional_pid, int optional_timeline_id) {
  ioctl_args arg;
  if (optional_pid < 0 ||
      (tracer_type != TRACER_TYPE_INS_VT &&
       tracer_type != TRACER_TYPE_APP_VT) ||
      (registration_type < SIMPLE_REGISTRATION ||
       registration_type > REGISTRATION_W_CONTROL_THREAD)) {
    printf("Tracer registration: incorrect parameters for tracer registration: %d\n",
          tracer_id);
    return -1;
  }
  InitIoctlArg(&arg);
  if (registration_type == SIMPLE_REGISTRATION) {
    sprintf(arg.cmd_buf, "%d,%d,%d,%d", tracer_id, tracer_type,
            SIMPLE_REGISTRATION, optional_timeline_id);
  } else {
    sprintf(arg.cmd_buf, "%d,%d,%d,%d,%d", tracer_id, tracer_type,
            registration_type, optional_timeline_id, optional_pid);
  }
  return SendToVtModule(VT_REGISTER_TRACER, &arg);
}


int getAssignedTracerID() {
  ioctl_args arg;
  InitIoctlArg(&arg);
  return (int)SendToVtModule(VT_GET_ASSIGNED_TRACER_ID, &arg);

}


int updateTracerClock(int tracer_id, s64 increment) {
  if (tracer_id < 0 || increment < 0) {
    printf("Update tracer clock: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  InitIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%d", tracer_id);
  arg.cmd_value = increment;
  return SendToVtModule(VT_UPDATE_TRACER_CLOCK, &arg);
}


int waitForExit(int tracer_id) {
  if (tracer_id < 0) {
    printf("Update tracer clock: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  InitIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%d", tracer_id);
  arg.cmd_value = 0;
  return SendToVtModule(VT_WAIT_FOR_EXIT, &arg);
}


s64 writeTracerResults(int tracer_id, int* results, int num_results) {
  if (tracer_id < 0 || num_results < 0) {
    printf("Write tracer results: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  InitIoctlArg(&arg);

  if (num_results == 0)
    sprintf(arg.cmd_buf, "%d", tracer_id);
  else {
    sprintf(arg.cmd_buf, "%d,", tracer_id);
    if (AppendToIoctlArg(&arg, results, num_results) < 0) return -1;
  }

  return SendToVtModule(VT_WRITE_RESULTS, &arg);
}


s64 getCurrentVirtualTime() {
  return SendToVtModule(VT_GET_CURRENT_VIRTUAL_TIME, NULL);
}


s64 getCurrentTimeTracer(int tracer) {
  ioctl_args arg;
  InitIoctlArg(&arg);
  if (tracer < 0) {
    printf("getCurrentTimeTracer: incorrect id: %d\n", tracer);
    return -1;
  }
  sprintf(arg.cmd_buf, "%d", tracer);
  return SendToVtModule(VT_GETTIME_TRACER, &arg);
}


int addProcessesToTracerSq(int tracer_id, int* pids, int num_pids) {
  if (tracer_id < 0 || num_pids <= 0) {
    printf("addProcessesToTracerSq: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  InitIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%d,", tracer_id);
  if (AppendToIoctlArg(&arg, pids, num_pids) < 0) return -1;

  return SendToVtModule(VT_ADD_PROCESSES_TO_SQ, &arg);
}


int initializeExp(int num_expected_tracers) {
  ioctl_args arg;
  InitIoctlArg(&arg);
  if (num_expected_tracers < 0) {
    printf("initializeExp: num expected tracers: %d < 0 !\n",
          num_expected_tracers);
    return -1;
  }
  sprintf(arg.cmd_buf, "%d,0,%d",EXP_CBE, num_expected_tracers);
  return SendToVtModule(VT_INITIALIZE_EXP, &arg);
}


int initializeVtExp(int exp_type, int num_timelines,
                    int num_expected_tracers) {
  ioctl_args arg;
  InitIoctlArg(&arg);
  if (num_expected_tracers < 0) {
    printf("InitializeVtExp: num expected tracers: %d < 0 !\n",
          num_expected_tracers);
    return -1;
  }

  if (exp_type != EXP_CS && exp_type != EXP_CBE) {
    printf("InitializeVtExp: Incorrect Experiment type !\n");
    return -1;
  }

  if (num_timelines <= 0) {
    printf("InitializeVtExp: Number of timelines cannot be <= 0\n");
    return -1;
  }

  sprintf(arg.cmd_buf, "%d,%d,%d", exp_type, num_timelines,
                                   num_expected_tracers);

  return SendToVtModule(VT_INITIALIZE_EXP, &arg);
}


int synchronizeAndFreeze() {
  ioctl_args arg;
  InitIoctlArg(&arg);
  return SendToVtModule(VT_SYNC_AND_FREEZE, &arg);
}


int stopExp() {
  ioctl_args arg;
  InitIoctlArg(&arg);
  return SendToVtModule(VT_STOP_EXP, &arg);
}


int progressBy(s64 duration, int num_rounds) {
  if (num_rounds <= 0)
     num_rounds = 1;
  ioctl_args arg;
  InitIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%d,", num_rounds);
  arg.cmd_value = duration;
  return SendToVtModule(VT_PROGRESS_BY, &arg);
}


int progressTimelineBy(int timeline_id, s64 duration) {
  if (timeline_id < 0 || duration <= 0) {
    printf("progress_timeline_By: incorrect arguments !\n");
    return -1;
  }
  ioctl_args arg;
  InitIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%d,", timeline_id);
  arg.cmd_value = duration;
  return SendToVtModule(VT_PROGRESS_TIMELINE_BY, &arg);

}


int setNetDeviceOwner(int tracer_id, char* intf_name) {
  ioctl_args arg;
  InitIoctlArg(&arg);
  if (tracer_id < 0 || !intf_name || strlen(intf_name) > IFNAMESIZ) {
    printf("Set net device owner: incorrect arguments for tracer: %d!\n",
          tracer_id);
    return -1;
  }
  sprintf(arg.cmd_buf, "%d,%s", tracer_id, intf_name);
  return SendToVtModule(VT_SET_NETDEVICE_OWNER, &arg);
}
