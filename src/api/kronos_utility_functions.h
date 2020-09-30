#ifndef __UTILITY_FNS_H
#define __UTILITY_FNS_H

#include <stdint.h>
#include <sys/ioctl.h>

#define EXP_CBE 1
#define EXP_CS 2

#define SUCCESS 1
#define FAILURE 0

#define MAX_API_ARGUMENT_SIZE 100
#define BUF_MAX_SIZE MAX_API_ARGUMENT_SIZE

#define MAX_PAYLOAD BUF_MAX_SIZE
#define NETLINK_USER 31
#define IFNAMESIZ 16

#define TRACER_TYPE_INS_VT 1
#define TRACER_TYPE_APP_VT 2

#define SIMPLE_REGISTRATION 0
#define REGISTRATION_W_SPINNER 1
#define REGISTRATION_W_CONTROL_THREAD 2

#define SMALLEST_PROCESS_QUANTA_INSNS 100000

typedef long long s64;

typedef struct ioctl_args_struct {
  char cmd_buf[BUF_MAX_SIZE];
  s64 cmd_value;
} ioctl_args;

#define VT_IOC_MAGIC 'k'

#define VT_UPDATE_TRACER_CLOCK _IOW(VT_IOC_MAGIC, 1, int)
#define VT_WRITE_RESULTS _IOW(VT_IOC_MAGIC, 2, int)
#define VT_GET_CURRENT_VIRTUAL_TIME _IOW(VT_IOC_MAGIC, 3, int)
#define VT_REGISTER_TRACER _IOW(VT_IOC_MAGIC, 4, int)
#define VT_ADD_PROCESSES_TO_SQ _IOW(VT_IOC_MAGIC, 5, int)
#define VT_SYNC_AND_FREEZE _IOW(VT_IOC_MAGIC, 6, int)
#define VT_INITIALIZE_EXP _IOW(VT_IOC_MAGIC, 7, int)
#define VT_GETTIME_TRACER _IOW(VT_IOC_MAGIC, 8, int)
#define VT_STOP_EXP _IOW(VT_IOC_MAGIC, 9, int)
#define VT_PROGRESS_BY _IOW(VT_IOC_MAGIC, 10, int)
#define VT_PROGRESS_TIMELINE_BY _IOW(VT_IOC_MAGIC, 11, int)
#define VT_WAIT_FOR_EXIT _IOW(VT_IOC_MAGIC, 12, int)
#define VT_GET_ASSIGNED_TRACER_ID _IOW(VT_IOC_MAGIC, 13, int)
#define VT_SET_NETDEVICE_OWNER _IOW(VT_IOC_MAGIC, 14, int)


//! Sends the command to the virtual time module as an ioctl call
s64 SendToVtModule(unsigned int cmd, ioctl_args* arg);

//! Returns the threadID of the invoking process
int gettid(void);

//! Returns 1 if run with root permissions
int IsRoot(void);

//! Checks if Kronos is loaded
int IsModuleLoaded(void);

//! Initializes/zeros out an ioct_args structure 
void InitIoctlArg(ioctl_args* arg);

//! Zeros out a string buffer of the specified size
void FlushBuffer(char* buf, int size);

//! Returns number of digits in the integer b
int NumCharacters(int n);

//! Converts an integer list of values into a comma separated string and adds
//  them to an ioctl_args structure
int AppendToIoctlArg(ioctl_args* arg, int* append_values, int num_values);

#endif
