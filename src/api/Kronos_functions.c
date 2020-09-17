
#include "Kronos_functions.h"
#include "Kronos_definitions.h"
#include "utility_functions.h"

#include <sys/ioctl.h>

#define TK_IOC_MAGIC  'k'
#define TK_IO_GET_STATS _IOW(TK_IOC_MAGIC,  1, int)
#define TK_IO_WRITE_RESULTS _IOW(TK_IOC_MAGIC,  2, int)
#define TK_IO_RESUME_BLOCKED_SYSCALLS _IOW(TK_IOC_MAGIC,  3, int)



int hello() {
	printf("Hello there from shared lib !\n");
	return 0;
}


int get_experiment_stats(ioctl_args * args) {

	if (is_root() && isModuleLoaded()) {
		return get_stats(args);
	}
	return -1;


}

/*
Register Tracer and create a spinner task for the tracer
*/
int addToExp_sp(float relative_cpu_speed, u32 n_round_instructions,
                pid_t pid ) {

	if (n_round_instructions <= 0 || relative_cpu_speed <= 0.0)
		return -1;

	int rel_cpu_speed = (int)(1000.0 * relative_cpu_speed);

	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,%d,1,%d", REGISTER_TRACER, rel_cpu_speed,
		        (int)n_round_instructions, pid);
		return send_to_kronos(command);
	}
	return -1;
}

int addToExp_child(float relative_cpu_speed, u32 n_round_instructions,
                pid_t pid ) {

	if (n_round_instructions <= 0 || relative_cpu_speed <= 0.0)
		return -1;

	int rel_cpu_speed = (int)(1000.0 * relative_cpu_speed);

	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,%d,2,%d", REGISTER_TRACER, rel_cpu_speed,
		        (int)n_round_instructions, pid);
		return send_to_kronos(command);
	}
	return -1;
}

/*
Register Tracer and do not create a spinner task for the tracer
*/
int addToExp(float relative_cpu_speed, u32 n_round_instructions) {

	if (n_round_instructions <= 0 || relative_cpu_speed <= 0.0)
		return -1;

	int rel_cpu_speed = (int)(1000.0 * relative_cpu_speed);

	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,%d,0,", REGISTER_TRACER, rel_cpu_speed,
		        (int)n_round_instructions);
		return send_to_kronos(command);
	}
	return -1;
}

/*
Starts a CBE Experiment
*/
int startExp() {
	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,", START_EXP);
		return send_to_kronos(command);
	}
	return -1;
}

int initializeExp(int exp_type) {

	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d", INITIALIZE_EXP, exp_type);
		return send_to_kronos(command);
	}
	return -1;

}

/*
Given all Pids added to experiment, will set all their virtual times to be the same,
then freeze them all (CBE and CS)
*/
int synchronizeAndFreeze(int n_expected_tracers) {

	if (n_expected_tracers <= 0)
		return -1;
	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,", SYNC_AND_FREEZE, n_expected_tracers);
		return send_to_kronos(command);
	}
	return -1;
}


/*
Stop a running experiment (CBE or CS) **Do not call stopExp if you are waiting
for a s3fProgress to return!!**
*/
int stopExp() {
	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,", STOP_EXP);
		return send_to_kronos(command);
	}
	return -1;
}


int update_tracer_params(int tracer_pid, float relative_cpu_speed,
                         u32 n_round_instructions) {
	if (tracer_pid <= 0 || relative_cpu_speed <= 0.0
	        || n_round_instructions == 0)
		return -1;

	int rel_cpu_speed = (int)(1000.0 * relative_cpu_speed);


	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,%d,%d,", UPDATE_TRACER_PARAMS, tracer_pid,
		        rel_cpu_speed, (int)n_round_instructions);
		return send_to_kronos(command);
	}
	return -1;
}

int set_netdevice_owner(int tracer_pid, char * intf_name) {
	if (tracer_pid <= 0 || intf_name == NULL || strlen(intf_name) > IFNAMESIZ)
		return -1;


	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,%s", SET_NETDEVICE_OWNER, tracer_pid,
		        intf_name);
		return send_to_kronos(command);
	}

	return -1;

}

int add_netdevice_to_vt_control(char * intf_name) {
	if (intf_name == NULL || strlen(intf_name) > IFNAMESIZ)
		return -1;


	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%s", ADD_NETDEVICE_TO_VT_CONTROL, intf_name);
		return send_to_kronos(command);
	}

	return -1;

}

int gettimepid(int pid) {

	if (pid <= 0)
		return -1;


	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,", GETTIMEPID, pid);
		return send_to_kronos(command);
	}

	return -1;
}


int progress_n_rounds(int n_rounds) {

	if (n_rounds <= 0)
		return -1;

	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,%d,", PROGRESS_N_ROUNDS, n_rounds);
		return send_to_kronos(command);
	}

	return -1;
}
int progress() {
	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,", PROGRESS);
		return send_to_kronos(command);
	}
	return -1;
}

int fire_timers() {
	if (isModuleLoaded()) {
		char command[100];
		flush_buffer(command, 100);
		sprintf(command, "%c,", RUN_DILATED_HRTIMERS);
		return send_to_kronos(command);
	}
	return -1;
}

int write_tracer_results(char * result) {

	if (result == NULL || strlen(result) > MAX_BUF_SIZ)
		return -1;


	if (isModuleLoaded()) {
	    int fp = open("/proc/status", O_RDWR);
	    int ret = 0;
	    if (fp < 0) {
		printf("Error communicating with Kronos\n");
		return -1;
	    }
	    ret = ioctl(fp, TK_IO_WRITE_RESULTS, result);
	    close(fp);
	    return ret;
	}

	return -1;
}
