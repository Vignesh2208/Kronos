#include "vt_module.h"
#include "common.h"
#include "sync_experiment.h"

/*
Has basic functionality for the Kernel Module itself. It defines how the
userland process communicates with the kernel module,
as well as what should happen when the kernel module is initialized and removed.
*/

/** LOCAL DECLARATIONS **/
int tracer_num = 0;
int EXP_CPUS = 0;
int TOTAL_CPUS = 0;
int experiment_status;
int experiment_type = NOT_SET;
int initialization_status;
s64 expected_time = 0;
s64 virt_exp_start_time = 0;
int current_progress_n_rounds = 0;
int total_completed_rounds = 0;
int total_num_timelines = 0;
int total_expected_tracers;
// locks
struct mutex exp_lock;
struct mutex file_lock;

// pointers
int *per_timeline_chain_length;
llist *per_timeline_tracer_list;
s64 *tracer_clock_array = NULL;
s64 *aligned_tracer_clock_array = NULL;
static struct proc_dir_entry *dilation_file = NULL;
struct task_struct *loop_task;
struct task_struct ** chaintask;

// hashmaps
hashmap get_tracer_by_id;
hashmap get_tracer_by_pid;

// atomic variables
atomic_t n_workers_running = ATOMIC_INIT(0);
atomic_t n_waiting_tracers = ATOMIC_INIT(0);

/** EXTERN DECLARATIONS **/

extern wait_queue_head_t progress_sync_proc_wqueue;
extern timeline * timeline_info;

loff_t vtllSeek(struct file *filp, loff_t pos, int whence);
int vtRelease(struct inode *inode, struct file *filp);
int vtMmap(struct file *filp, struct vm_area_struct *vma);
long vtIoctl(struct file *filp, unsigned int cmd, unsigned long arg);
ssize_t vtWrite(struct file *file, const char __user *buffer, size_t count, loff_t *data);


static const struct file_operations proc_file_fops = {
    .unlocked_ioctl = vtIoctl,
    .llseek = vtllSeek,
    .release = vtRelease,
    .write = vtWrite,
    .mmap = vtMmap,
    .owner = THIS_MODULE,
};


//! Gets the PID of our synchronizer spinner task (only in 64 bit)
int getSpinnerPid(struct subprocess_info *info, struct cred *new) {
  loop_task = current;
  printk(KERN_INFO "Kronos: Loop Task Started. Pid: %d\n", current->pid);
  return 0;
}

/*
 * Hack to get 64 bit running correctly. Starts a process that will just loop
 * while the experiment is going on. Starts an executable specified in the path
 * in user space from kernel space.
 */
int RunUserModeSynchronizerProcess(char *path, char **argv, char **envp,
                                      int wait) {
  struct subprocess_info *info;
  gfp_t gfp_mask = (wait == UMH_NO_WAIT) ? GFP_ATOMIC : GFP_KERNEL;

  info = call_usermodehelper_setup(path, argv, envp, gfp_mask, getSpinnerPid,
                                   NULL, NULL);
  if (info == NULL) return -ENOMEM;

  return call_usermodehelper_exec(info, wait);
}

//! Do not allow seek in /proc/dilation/status file
loff_t vtllSeek(struct file *filp, loff_t pos, int whence) { return 0; }

//! Do not do anything upon close on /proc/dilation/status file
int vtRelease(struct inode *inode, struct file *filp) { return 0; }

//! Handle memory map operations onto the /proc/dilation/status file
int vtMmap(struct file *filp, struct vm_area_struct *vma) {
  if (aligned_tracer_clock_array) {
    unsigned long pfn =
        virt_to_phys((void *)aligned_tracer_clock_array) >> PAGE_SHIFT;
    unsigned long len = vma->vm_end - vma->vm_start;
    int ret;

    ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
    if (ret < 0) {
      pr_err("could not map the address area\n");
      return -EIO;
    }
  } else {
    return -EINVAL;
  }
  return 0;
}

//! Handling write operations to /proc/dilation/status file
ssize_t vtWrite(struct file *file, const char __user *buffer, size_t count,
                 loff_t *data) {
  char write_buffer[MAX_API_ARGUMENT_SIZE];
  unsigned long buffer_size;
  int i = 0;
  int ret = 0;
  int num_integer_args;
  int api_integer_args[MAX_API_ARGUMENT_SIZE];
  int tracer_id;
  tracer *curr_tracer;

   memset(api_integer_args, 0, sizeof(int) * MAX_API_ARGUMENT_SIZE);

   if(count > MAX_API_ARGUMENT_SIZE) {
    buffer_size = MAX_API_ARGUMENT_SIZE;
  } else {
    buffer_size = count;
  }

  for(i = 0; i < MAX_API_ARGUMENT_SIZE; i++)
    write_buffer[i] = '\0';
  
  if(copy_from_user(write_buffer, buffer, buffer_size))
    return -EINVAL;


  /* Use +2 to skip over the first two characters (i.e. the switch and the ,) */
  if (write_buffer[0] == VT_ADD_TO_SQ) {
    if (initialization_status != INITIALIZED &&
        experiment_status == STOPPING) {
      PDEBUG_E(
        "VT_ADD_PROCESSES_TO_SQ: Operation cannot be performed when "
        "experiment is not initialized !");
      return -EINVAL;
    }

    num_integer_args = ConvertStringToArray(
      write_buffer + 2, api_integer_args, MAX_API_ARGUMENT_SIZE);

    if (num_integer_args <= 1) {
      PDEBUG_E("VT_ADD_PROCESS_TO_SQ: Not enough arguments !");
      return -EINVAL;
    }

    tracer_id = api_integer_args[0];
    if (tracer_id != -1)
        curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
    else
        curr_tracer = hmap_get_abs(&get_tracer_by_pid, current->pid);
    if (!curr_tracer) {
      PDEBUG_I("VT_ADD_PROCESS_TO_SQ: Tracer : %d, not registered\n", tracer_id);
      return -EINVAL;
    }

    GetTracerStructWrite(curr_tracer);
    for (i = 1; i < num_integer_args; i++) {
      AddToTracerScheduleQueue(curr_tracer, api_integer_args[i]);
    }
    PutTracerStructWrite(curr_tracer);
    return 0;

  } else if (write_buffer[0] == VT_WRITE_RES) {

    if (initialization_status != INITIALIZED)
      return -EINVAL;

    if (current->associated_tracer_id <= 0) {
      PDEBUG_E("VT_WRITE_RES: Process: %d is not associated with any tracer !\n",
               current->pid);
      return -EINVAL;
    }

    num_integer_args = ConvertStringToArray(
      write_buffer + 2, api_integer_args, MAX_API_ARGUMENT_SIZE);

    if (num_integer_args < 1) {
      PDEBUG_E("VT_WRITE_RES: Not enough arguments !");
      return -EINVAL;
    }

    tracer_id = api_integer_args[0];
    if (tracer_id != -1)
        curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
    else
        curr_tracer = hmap_get_abs(&get_tracer_by_pid, current->pid);
    if (!curr_tracer) {
      PDEBUG_I("VT_WRITE_RES: Tracer : %d, not registered\n", current->pid);
      return -EINVAL;
    }

    current->ready = 1;

    HandleTracerResults(curr_tracer, &api_integer_args[1],
                        num_integer_args - 1);

    wait_event_interruptible(
      *curr_tracer->w_queue,
      current->associated_tracer_id <= 0 ||
          (curr_tracer->w_queue_wakeup_pid == current->pid &&
           current->burst_target > 0));
    PDEBUG_V(
      "VT_WRITE_RES: Associated Tracer : %d, Process: %d, "
      "resuming from wait. Ptrace_msteps = %d\n",
      tracer_id, current->pid, current->ptrace_msteps);

    current->ready = 0;

    // Ensure that for INS_VT tracer, only the tracer process can invoke this
    // call
    if (current->associated_tracer_id)
      BUG_ON(curr_tracer->tracer_task->pid != current->pid &&
             curr_tracer->tracer_type == TRACER_TYPE_INS_VT);

    if (current->associated_tracer_id <= 0) {
      current->burst_target = 0;
      current->virt_start_time = 0;
      current->curr_virt_time = 0;
      current->associated_tracer_id = 0;
      current->wakeup_time = 0;
      current->vt_exec_task = NULL;
      current->tracer_clock = NULL;
      PDEBUG_I("VT_WRITE_RES: Tracer: %d, Process: %d STOPPING\n", tracer_id,
         current->pid);
      return 0;
    }

    PDEBUG_V("VT_WRITE_RES: Tracer: %d, Process: %d Returning !\n", tracer_id,
           current->pid);
    return (ssize_t) current->burst_target;

  }
  else {
    PDEBUG_E("VT_WRITE_RES: Invalid Write Command: %s\n", write_buffer);
    return -EINVAL;
  }
  return 0;
}

long vtIoctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  int err = 0;
  int retval = 0;
  int i = 0;
  uint8_t mask = 0;
  int num_rounds = 0;
  unsigned long flags;
  overshoot_info *args;
  invoked_api *api_info;
  struct timeval *tv;
  overshoot_info tmp;
  invoked_api api_info_tmp;
  char *ptr;
  int ret, num_integer_args;
  int api_integer_args[MAX_API_ARGUMENT_SIZE];
  tracer *curr_tracer;
  int tracer_id;

  memset(api_info_tmp.api_argument, 0, sizeof(char) * MAX_API_ARGUMENT_SIZE);
  memset(api_integer_args, 0, sizeof(int) * MAX_API_ARGUMENT_SIZE);

  PDEBUG_V("VT-IO: Got ioctl from : %d\n", current->pid);

  /*
   * extract the type and number bitfields, and don't decode
   * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
   */
  if (_IOC_TYPE(cmd) != VT_IOC_MAGIC) return -ENOTTY;

  /*
   * the direction is a bitmask, and VERIFY_WRITE catches R/W
   * transfers. `Type' is user-oriented, while
   * access_ok is kernel-oriented, so the concept of "read" and
   * "write" is reversed
   */
  if (_IOC_DIR(cmd) & _IOC_READ)
    err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
  else if (_IOC_DIR(cmd) & _IOC_WRITE)
    err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

  if (err) return -EINVAL;

  switch (cmd) {
    case VT_UPDATE_TRACER_CLOCK:
      // Only registered tracer process can invoke this ioctl

      if (initialization_status != INITIALIZED ||
          experiment_status == NOTRUNNING) {
        PDEBUG_E(
            "VT_UPDATE_TRACER_CLOCK: Operation cannot be performed when "
            "experiment is not running !");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api))) {
        return -EINVAL;
      }

      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args != 1) {
        PDEBUG_E(
            "VT_UPDATE_TRACER_CLOCK: Not enough arguments. Missing tracer_id "
            "!");
        return -EINVAL;
      }

      tracer_id = api_integer_args[0];
      if (tracer_id != -1)
          curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
      else
          curr_tracer = hmap_get_abs(&get_tracer_by_pid, current->pid);
      if (!curr_tracer) {
        PDEBUG_I("VT-IO: Tracer : %d, not registered\n", tracer_id);
        return -EINVAL;
      }

      // Noop for APP VT tracers and its associated processes
      if (curr_tracer->tracer_type == TRACER_TYPE_APP_VT ||
          current->associated_tracer_id <= 0)
        return 0;

      curr_tracer->curr_virtual_time += api_info_tmp.return_value;
      SetChildrenTime(curr_tracer, curr_tracer->tracer_task,
                      curr_tracer->curr_virtual_time, 0);
      return 0;

    case VT_WRITE_RESULTS:
      // A registered tracer process or one of the processes in a tracer's
      // schedule queue can invoke this ioctl For INSVT tracer type, only the
      // tracer process can invoke this ioctl

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_WRITE_RESULTS: Operation cannot be performed when experiment "
            "is not initialized !");
        return -EINVAL;
      }

      if (current->associated_tracer_id <= 0) {
        PDEBUG_E("Process: %d is not associated with any tracer !\n");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api))) {
        return -EINVAL;
      }

      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args < 1) {
        PDEBUG_E("VT_WRITE_RESULTS: Not enough arguments !");
        return -EINVAL;
      }

      tracer_id = api_integer_args[0];
      if (tracer_id != -1)
          curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
      else
          curr_tracer = hmap_get_abs(&get_tracer_by_pid, current->pid);
      if (!curr_tracer) {
        PDEBUG_I("VT-IO: Tracer : %d, not registered\n", current->pid);
        return -EINVAL;
      }

      current->ready = 1;

      mutex_lock(&file_lock);
      HandleTracerResults(curr_tracer, &api_integer_args[1],
                          num_integer_args - 1);
      mutex_unlock(&file_lock);

      wait_event_interruptible(
          *curr_tracer->w_queue,
          current->associated_tracer_id <= 0 ||
              (curr_tracer->w_queue_wakeup_pid == current->pid &&
               current->burst_target > 0));
      PDEBUG_V(
          "VT-IO: Associated Tracer : %d, Process: %d, resuming from wait\n",
          tracer_id, current->pid);

      current->ready = 0;

      // Ensure that for INS_VT tracer, only the tracer process can invoke this
      // call
      if (current->associated_tracer_id)
        BUG_ON(curr_tracer->tracer_task->pid != current->pid &&
               curr_tracer->tracer_type == TRACER_TYPE_INS_VT);

      if (current->associated_tracer_id <= 0) {
        current->burst_target = 0;
        current->virt_start_time = 0;
        current->curr_virt_time = 0;
        current->associated_tracer_id = 0;
        current->wakeup_time = 0;
        current->vt_exec_task = NULL;
        current->tracer_clock = NULL;
        api_info_tmp.return_value = 0;
        if (copy_to_user(api_info, &api_info_tmp, sizeof(invoked_api))) {
          PDEBUG_I(
              "Status Read: Tracer : %d, Process: %d Resuming from wait. "
              "Error copying to user buf\n",
              tracer_id, current->pid);
          return -EINVAL;
        }
        PDEBUG_I("VT-IO: Tracer: %d, Process: %d STOPPING\n", tracer_id,
                 current->pid);

        return 0;
      }

      api_info_tmp.return_value = current->burst_target;
      if (copy_to_user(api_info, &api_info_tmp, sizeof(invoked_api))) {
        PDEBUG_I(
            "Status Read: Tracer : %d, Process: %d Resuming from wait. "
            "Error copying to user buf\n",
            tracer_id, current->pid);
        return -EINVAL;
      }

      PDEBUG_V("VT-IO: Tracer: %d, Process: %d Returning !\n", tracer_id,
               current->pid);
      return 0;

    case VT_GET_CURRENT_VIRTUAL_TIME:
      // Any process can invoke this call.
      tv = (struct timeval *)arg;
      if (!tv) return -EINVAL;

      struct timeval ktv;
      if (expected_time == 0)
        do_gettimeofday(&ktv);
      else {
        ktv = ns_to_timeval(expected_time);
      }

      if (copy_to_user(tv, &ktv, sizeof(ktv))) return -EINVAL;

      return 0;

    case VT_REGISTER_TRACER:
      // Any process can invoke this call if it is not already associated with a
      // tracer

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_REGISTER_TRACER: Operation cannot be performed when experiment "
            "is not initialized !");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api)))
        return -EINVAL;

      if (current->associated_tracer_id) {
        PDEBUG_E(
            "Process: %d, already associated with another tracer. "
            "It cannot be registered as a new tracer !",
            current->pid);
        return -EINVAL;
      }

      retval = RegisterTracerProcess(api_info_tmp.api_argument);
      if (retval == FAIL) {
        PDEBUG_E("Tracer Registration failed for tracer: %d\n", current->pid);
        return -EINVAL;
      }
      api_info_tmp.return_value = retval;

      if (copy_to_user(api_info, &api_info_tmp, sizeof(invoked_api))) {
        PDEBUG_I(
            "VT_REGISTER_TRACER: Tracer : %d, "
            "Error copying to user buf\n",
            current->pid);
        return -EINVAL;
      }
      atomic_inc(&n_waiting_tracers);
      wake_up_interruptible(&progress_sync_proc_wqueue);
      return 0;

    case VT_ADD_PROCESSES_TO_SQ:
      // Any process can invoke this call.

      if (initialization_status != INITIALIZED &&
          experiment_status == STOPPING) {
        PDEBUG_E(
            "VT_ADD_PROCESSES_TO_SQ: Operation cannot be performed when "
            "experiment is not initialized !");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api)))
        return -EINVAL;

      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args <= 1) {
        PDEBUG_E("VT_ADD_PROCESS_TO_SQ: Not enough arguments !");
        return -EINVAL;
      }

      tracer_id = api_integer_args[0];
      if (tracer_id != -1)
          curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
      else
          curr_tracer = hmap_get_abs(&get_tracer_by_pid, current->pid);
      if (!curr_tracer) {
        PDEBUG_I("VT_ADD_PROCESS_TO_SQ: Tracer : %d, not registered\n", tracer_id);
        return -EINVAL;
      }

      GetTracerStructWrite(curr_tracer);
      for (i = 1; i < num_integer_args; i++) {
        AddToTracerScheduleQueue(curr_tracer, api_integer_args[i]);
      }
      PutTracerStructWrite(curr_tracer);
      return 0;

    case VT_SYNC_AND_FREEZE:
      // Any process can invoke this call.
      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_SYNC_AND_FREEZE: Operation cannot be performed when experiment "
            "is not initialized !");
        return -EINVAL;
      }

      if (experiment_status != NOTRUNNING) {
        PDEBUG_E(
            "VT_SYNC_AND_FREEZE: Operation cannot be performed when experiment "
            "is already running!");
        return -EINVAL;
      }

      return SyncAndFreeze();

    case VT_INITIALIZE_EXP:
      // Any process can invoke this call.

      if (initialization_status == INITIALIZED) {
        PDEBUG_E(
            "VT_INITIALIZE_EXP: Operation cannot be performed when experiment "
            "is already initialized !");
        return -EINVAL;
      }

      if (experiment_status != NOTRUNNING) {
        PDEBUG_E(
            "VT_INITIALIZE_EXP: Operation cannot be performed when experiment "
            "is already running!");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api))) {
        return -EINVAL;
      }
      return HandleInitializeExpCmd(api_info_tmp.api_argument);

    case VT_GETTIME_TRACER:
      // Any process can invoke this call.

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_GETTIME_PID: Operation cannot be performed when experiment is "
            "not initialized !");
        return -EINVAL;
      }

      if (experiment_status == NOTRUNNING) {
        PDEBUG_E(
            "VT_GETTIME_PID: Operation cannot be performed when experiment is "
            "not running!");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api))) {
        return -EINVAL;
      }

      tracer_id = api_info_tmp.api_argument;
      curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
      if (!curr_tracer) {
        PDEBUG_I("VT_GETTIME_TRACER: Tracer : %d, not registered\n", tracer_id);
        return -EINVAL;
      }
      api_info_tmp.return_value = curr_tracer->curr_virtual_time;
      if (copy_to_user(api_info, &api_info_tmp, sizeof(invoked_api))) {
        PDEBUG_I("VT_GETTIME_TRACER: Error copying to user buf\n");
        return -VT_GETTIME_TRACER;
      }
      return 0;

    case VT_STOP_EXP:
      // Any process can invoke this call.
      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_STOP_EXP: Operation cannot be performed when experiment is not "
            "initialized !");
        return -EINVAL;
      }

      if (experiment_status == NOTRUNNING) {
        PDEBUG_E(
            "VT_STOP_EXP: Operation cannot be performed when experiment is not "
            "running!");
        return -EINVAL;
      }

      return HandleStopExpCmd();

    case VT_PROGRESS_BY:
      // Any process can invoke this call.

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_PROGRESS_BY: Operation cannot be performed when experiment is "
            "not initialized !");
        return -EINVAL;
      }

      if (experiment_status == NOTRUNNING) {
        PDEBUG_E(
            "VT_PROGRESS_BY: Operation cannot be performed when experiment is "
            "not running!");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api)))
        return -EINVAL;


      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args < 1) {
        PDEBUG_E("VT_PROGRESS_BY: Not enough arguments !");
        return -EINVAL;
      }

      num_rounds = api_integer_args[0];
      return ProgressBy(api_info_tmp.return_value, num_rounds);

    case VT_PROGRESS_TIMELINE_BY:
      // Any process can invoke this call.

      if (initialization_status != INITIALIZED) {
        PDEBUG_I(
            "VT_PROGRESS_TIMELINE_BY: Operation cannot be performed when "
            "experiment is not initialized !");
        return -EFAULT;
      }

      if (experiment_status == NOTRUNNING) {
        PDEBUG_I(
            "VT_PROGRESS_TIMELINE_BY: Operation cannot be performed when "
            "experiment is not running!");
        return -EFAULT;
      }


      if (experiment_type != EXP_CS) {
        PDEBUG_A("Progress timeline cannot be called for EXP_CBE !\n");
        return -EFAULT;
      }


      api_info = (invoked_api *)arg;
      if (!api_info) return -EFAULT;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api))) {
        return -EFAULT;
      }

      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args < 1) {
        PDEBUG_I("VT_PROGRESS_TIMELINE_BY: Not enough arguments !");
        return -EFAULT;
      }
      return ProgressTimelineBy(api_integer_args[0], api_info_tmp.return_value);

    case VT_SET_NETDEVICE_OWNER:
      // Any process can invoke this call.

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_SET_NETDEVICE_OWNER: Operation cannot be performed when "
            "experiment is not initialized !");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api)))
        return -EINVAL;

      return HandleSetNetdeviceOwnerCmd(api_info_tmp.api_argument);

    case VT_WAIT_FOR_EXIT:

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_WAIT_FOR_EXIT: Operation cannot be performed when experiment "
            "is not initialized !");
        return -EINVAL;
      }

      if (current->associated_tracer_id <= 0) {
        PDEBUG_E("Process: %d is not associated with any tracer !\n");
        return -EINVAL;
      }

      api_info = (invoked_api *)arg;
      if (!api_info) return -EINVAL;
      if (copy_from_user(&api_info_tmp, api_info, sizeof(invoked_api)))
        return -EINVAL;

      num_integer_args = ConvertStringToArray(
          api_info_tmp.api_argument, api_integer_args, MAX_API_ARGUMENT_SIZE);

      if (num_integer_args < 1) {
        PDEBUG_E("VT_WAIT_FOR_EXIT: Not enough arguments !");
        return -EINVAL;
      }

      tracer_id = api_integer_args[0];
      curr_tracer = hmap_get_abs(&get_tracer_by_id, tracer_id);
      if (!curr_tracer) {
        PDEBUG_I("VT_WAIT_FOR_EXIT: Tracer : %d, not registered\n", current->pid);
        return -EINVAL;
      }

      wait_event_interruptible(*timeline_info[0].w_queue,
                               timeline_info[0].stopping == 1);
      PDEBUG_V(
          "VT_WAIT_FOR_EXIT: Associated Tracer : %d, Process: %d, "
          "resuming from wait for exit\n",
          tracer_id, current->pid);
      return 0;

    case VT_GET_ASSIGNED_TRACER_ID:

      if (initialization_status != INITIALIZED) {
        PDEBUG_E(
            "VT_GET_ASSIGNED_TRACER_ID: Operation cannot be performed when experiment "
            "is not initialized !");
        return -EINVAL;
      }
      return current->associated_tracer_id;

    default:
      return -ENOTTY;
  }

  return 0;
}

/*
 * This function gets executed when the kernel module is loaded.
 * It creates the file for process -> kernel module communication,
 * sets up mutexes, timers, and hooks the system call table.
 */
int __init myModuleInit(void) {
  int i;

  PDEBUG_A("Loading MODULE\n");

  /* Set up Kronos status file in /proc */
  dilation_file = proc_create(DILATION_FILE, 0666, NULL, &proc_file_fops);
  tracer_clock_array = NULL;
  aligned_tracer_clock_array = NULL;

  mutex_init(&file_lock);

  if (dilation_file == NULL) {
    PDEBUG_E("Error: Could not initialize /proc/%s\n", DILATION_FILE);
    return -ENOMEM;
  }
  PDEBUG_A(" /proc/%s created\n", DILATION_FILE);

  /* If it is 64-bit, initialize the looping script */
#ifdef __x86_64
  char *argv[] = {"/bin/x64_synchronizer", NULL};
  static char *envp[] = {"HOME=/", "TERM=linux",
                         "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
  RunUserModeSynchronizerProcess(argv[0], argv, envp, UMH_NO_WAIT);
#endif

  initialization_status = NOT_INITIALIZED;
  experiment_status = NOTRUNNING;

  /* Acquire number of CPUs on system */
  TOTAL_CPUS = num_online_cpus();
  PDEBUG_A("Total Number of CPUS: %d\n", num_online_cpus());

  if (TOTAL_CPUS > 2)
    EXP_CPUS = TOTAL_CPUS - 2;
  else
    EXP_CPUS = 1;

  expected_time = 0;

  PDEBUG_A("Number of EXP_CPUS: %d\n", EXP_CPUS);
  PDEBUG_A("MODULE LOADED SUCCESSFULLY \n");
  return 0;
}

/*
 * This function gets called when the kernel module is unloaded.
 * It frees up all memory and deletes timers.
 */
void __exit myModuleExit(void) {
  s64 i;
  int num_prev_alotted_pages;

  remove_proc_entry(DILATION_FILE, NULL);
  PDEBUG_A(" /proc/%s deleted\n", DILATION_FILE);

  if (tracer_num > 0) {
    // Free all tracer structs from previous experiment if any
    FreeAllTracers();
    if (tracer_clock_array) {
      num_prev_alotted_pages = (tracer_num * sizeof(s64)) / PAGE_SIZE;
      num_prev_alotted_pages++;
      FreeMmapPages(tracer_clock_array, num_prev_alotted_pages);
    }
    tracer_clock_array = NULL;
    aligned_tracer_clock_array = NULL;
    tracer_num = 0;
  }

  /* Kill the looping task */
#ifdef __x86_64
  if (loop_task != NULL) KillProcess(loop_task, SIGKILL);
#endif
  PDEBUG_A("MODULE UNLOADED\n");
}

/* Register the init and exit functions here so insmod can run them */
module_init(myModuleInit);
module_exit(myModuleExit);

/* Required by kernel */
MODULE_LICENSE("GPL");
