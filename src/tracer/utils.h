#ifndef __UTILS_H
#define __UTILS_H

#include "includes.h"

int runCommand(char * full_command_str, pid_t * child_pid);
void printTraceeList(llist * tracee_list);
int createSpinnerTask(pid_t * child_pid);

#endif
