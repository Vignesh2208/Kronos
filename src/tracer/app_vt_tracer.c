// APP-VT is an experimental feature which is not fully developed yet. It would use Intel-PIN-TOOL

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h> 
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

const char *homedir;


#define MAX_COMMAND_LENGTH 1000
#define MAX_CONTROLLABLE_PROCESSES 100
#define MAX_BUF_SIZ 1000
#define FAIL -1

#include "Kronos_functions.h"



//! Convert string to integer
int strToInteger(char *s) {
  int i, n;
  n = 0;
  for (i = 0; *(s + i) >= '0' && *(s + i) <= '9'; i++)
    n = 10 * n + *(s + i) - '0';
  return n;
}

//! Used when reading the input from Kronos module.
//  Will basically return the next number in a string
int getNextValue(char *write_buffer) {
  int i;
  for (i = 1; *(write_buffer + i) >= '0' && *(write_buffer + i) <= '9'; i++) {
    continue;
  }

  if (write_buffer[i] == '\0') return -1;

  if (write_buffer[i + 1] == '\0') return -1;

  return (i + 1);
}

//! Creates a continusly spinning task and returns its pid.
int createSpinnerTask(pid_t *child_pid) {
  pid_t child;

  child = fork();
  if (child == (pid_t)-1) {
    printf("fork() failed in createSpinnerTask: %s.\n", strerror(errno));
    exit(-1);
  }

  if (!child) {
    while (1) {
      usleep(1000000);
    }
    exit(2);
  }

  *child_pid = child;
  return 0;
}

//! Envelop orig command to be started under the control of intel-pin
void envelopeCommandUnderPin(char * enveloped_cmd_str, char * orig_command_str,
                             int total_num_tracers, int tracer_id) {
  if ((homedir = getenv("HOME")) == NULL) {
    homedir = getpwuid(getuid())->pw_dir;
  }
  char pin_binary_path[MAX_COMMAND_LENGTH];
  memset(pin_binary_path, 0, MAX_COMMAND_LENGTH);
  sprintf(pin_binary_path, "%s/Kronos/src/tracer/pintool/pin-3.13/pin", homedir);
  char * pintool_path = "/usr/lib/inscount_tls.so";
  memset(enveloped_cmd_str, 0, MAX_COMMAND_LENGTH);
  sprintf(enveloped_cmd_str, "%s -t %s -n %d -i %d -- %s", pin_binary_path,
          pintool_path, total_num_tracers, tracer_id, orig_command_str);
  printf("Full Enveloped Command: %s", enveloped_cmd_str);
  
} 


//! Fork exec the tracee command under the control of pin. It returns the pid
//  of the started child process.
int runCommandUnderPin(char *orig_command_str, pid_t *child_pid,
                       int total_num_tracers, int tracer_id) {
  char **args;
  char full_command_str[MAX_COMMAND_LENGTH];
  char *iter = full_command_str;

  int i = 0;
  int n_tokens = 0;
  int token_no = 0;
  int token_found = 0;
  int ret;
  pid_t child;

  envelopeCommandUnderPin(full_command_str, orig_command_str,
                          total_num_tracers, tracer_id);

  
  while (full_command_str[i] != '\0') {
    if (i != 0 && full_command_str[i - 1] != ' ' &&
        full_command_str[i] == ' ') {
      n_tokens++;
    }
    i++;
  }

  args = malloc(sizeof(char *) * (n_tokens + 2));

  if (!args) {
    printf("Malloc Error In runCommand\n");
    exit(-1);
  }
  args[n_tokens + 1] = NULL;

  i = 0;

  while (full_command_str[i] != '\n' && full_command_str[i] != '\0') {
    if (i == 0) {
      args[0] = full_command_str;
      token_no++;
      token_found = 1;
    } else {
      if (full_command_str[i] == ' ') {
        full_command_str[i] = '\0';
        token_found = 0;
      } else if (token_found == 0) {
        args[token_no] = full_command_str + i;
        token_no++;
        token_found = 1;
      }
    }
    i++;
  }
  full_command_str[i] = '\0';

  child = fork();
  if (child == (pid_t)-1) {
    printf("fork() Failed in runCommand: %s.\n", strerror(errno));
    exit(-1);
  }

  if (!child) {
    i = 0;
    while (args[i] != NULL) {
      if (strcmp(args[i], ">>") == 0 || strcmp(args[i], ">") == 0) {
        args[i] = '\0';
        
        int fd;

        if (args[i + 1]) {
          if ((fd = open(args[i + 1], O_RDWR | O_CREAT)) == -1) {
            perror("open");
            exit(-1);
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }
        break;
      }
      if (strcmp(args[i], "<") == 0) {
        args[i] = '\0';
        int fd;
        // printf("Stdin file : %s\n", args[i+1]);
        if (args[i + 1]) {
          if ((fd = open(args[i + 1], O_RDONLY)) < 0) {
            perror("open");
            exit(-1);
          }
          dup2(fd, 0);
          close(fd);
        }
      }
      i++;
    }
    fflush(stdout);
    fflush(stderr);
    execvp(args[0], &args[0]);
    free(args);
    exit(2);
  }

  *child_pid = child;
  return 0;
}

void printUsage(int argc, char* argv[]) {
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [ -h | --help ]\n", argv[0]);
  fprintf(stderr,	"		%s -i TRACER_ID -n TOTAL_NUM_TRACERS"
          "[-f CMDS_FILE_PATH or -c \"CMD with args\"] [-t TimelineID]"
          "-s CREATE_SPINNER\n", argv[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "This program executes all COMMANDs specified in the "
          "CMD file path or the specified CMD with arguments in trace mode "
          "under the control of VT-Module\n");
  fprintf(stderr, "\n");
}


int main(int argc, char * argv[]) {

  char * cmd_file_path = NULL;
  char * line;
  size_t line_read;
  size_t len = 0;
  pid_t new_cmd_pid;
  int tracer_id = -1;
  char command[MAX_BUF_SIZ];
  int cpu_assigned;
  int create_spinner = 0;
  pid_t spinned_pid;
  FILE* fp;
  int option = 0;
  int read_from_file = 1;
  int i, status;
  int total_num_tracers, num_controlled_processes;
  pid_t controlled_pids[MAX_CONTROLLABLE_PROCESSES];
  num_controlled_processes = 0;
  int timeline_id = 0;

  if (argc < 4 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    printUsage(argc, argv);
    exit(FAIL);
  }


  while ((option = getopt(argc, argv, "i:f:n:st:c:h")) != -1) {
    switch (option) {
    case 'i' : tracer_id = atoi(optarg);
      if (tracer_id <= 0) {
        fprintf(stderr, "Explicitly assigned tracer-id must be positive > 0\n");
        exit(FAIL);
      }
      break;
    case 'n' : total_num_tracers = atoi(optarg);
      break;
    case 'f' : cmd_file_path = optarg;
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

  
  if (read_from_file)
    printf("CMDS_FILE_PATH: %s\n", cmd_file_path);
  else
    printf("CMD TO RUN: %s\n", command);

  
  if (create_spinner) {
    createSpinnerTask(&spinned_pid);
    cpu_assigned = registerTracer(tracer_id, TRACER_TYPE_APP_VT,
                                  REGISTRATION_W_SPINNER, spinned_pid,
                                  timeline_id);
  } else
    cpu_assigned = registerTracer(tracer_id, TRACER_TYPE_APP_VT,
                                  SIMPLE_REGISTRATION, 0, timeline_id);

  if (cpu_assigned < 0) {
    printf("TracerID: %d, Registration Error. Errno: %d\n", tracer_id, errno);
    exit(FAIL);
  }
  if (tracer_id == -1) {
       tracer_id = getAssignedTracerID();
       if (tracer_id <= 0) {
           fprintf(stderr, "This shouldn't happen. Auto assigned tracer-id is negative ! Make sure experiment is initialized before starting a tracer.");
           exit(FAIL);
       }
  }
  printf("Tracer PID: %d\n", (pid_t)getpid());
  printf("Tracer ID: %d\n", tracer_id);


  if (read_from_file) {
    fp = fopen(cmd_file_path, "r");
    if (fp == NULL) {
      printf("ERROR Opening Cmds File\n");
      exit(FAIL);
    }
    while ((line_read = getline(&line, &len, fp)) != -1) {
      num_controlled_processes ++;
      runCommandUnderPin(line, &new_cmd_pid, total_num_tracers, tracer_id);
      controlled_pids[num_controlled_processes - 1] = new_cmd_pid;
      printf("TracerID: %d, Started Command: %s", tracer_id, line);
    }
    fclose(fp);
  } else {
    
    num_controlled_processes ++;
    runCommandUnderPin(command, &new_cmd_pid, total_num_tracers, tracer_id);
    controlled_pids[num_controlled_processes - 1] = new_cmd_pid;
    printf("TracerID: %d, Started Command: %s", tracer_id, command);
  }

  
  // Block here with a call to VT-Module
  printf("Waiting for Exit !\n");
  fflush(stdout);
  if (waitForExit(tracer_id) < 0) {
    printf("ERROR: Wait for Exit. Thats not good !\n");
  }
  // Resume from Block. Send KILL Signal to each child. TODO: Think of a more gracefull way to do this.
  printf("Resumed from Wait for Exit! Waiting for processes to finish !\n");
  fflush(stdout);
  for(i = 0; i < num_controlled_processes; i++) {
    kill(controlled_pids[i], SIGKILL);
    waitpid(controlled_pids[i], &status, 0);
  } 

  printf("Exiting Tracer ...\n");
  return 0;
}
