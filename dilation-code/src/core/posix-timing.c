#include "dilation_module.h"


/*
Contains the code for acquiring the syscall table, as well as the 4 system calls Timekeeper currently hooks.
*/

extern unsigned long **aquire_sys_call_table(void);
extern s64 get_dilated_time(struct task_struct * task);

extern int experiment_stopped;
extern s64 Sim_time_scale;
extern struct list_head exp_list;


asmlinkage long sys_clock_nanosleep_new(const clockid_t which_clock, int flags, const struct timespec __user * rqtp, struct timespec __user * rmtp);
asmlinkage int sys_clock_gettime_new(const clockid_t which_clock, struct timespec __user * tp);
asmlinkage long (*ref_sys_clock_nanosleep)(const clockid_t which_clock, int flags, const struct timespec __user * rqtp, struct timespec __user * rmtp);
asmlinkage int (*ref_sys_clock_gettime)(const clockid_t which_clock, struct timespec __user * tp);



asmlinkage long sys_clock_nanosleep_new(const clockid_t which_clock, int flags, const struct timespec __user * rqtp, struct timespec __user * rmtp) {

	struct list_head *pos;
	struct list_head *n;
	struct dilation_task_struct* task;
	struct dilation_task_struct *dilTask;
	struct timeval ktv;
	struct task_struct *current_task;
	s64 now;
	s64 now_new;
	s32 rem;
	s64 real_running_time;
	s64 dilated_running_time;
	current_task = current;
	struct sleep_helper_struct * sleep_helper;

	spin_lock(&current->dialation_lock);
	if (experiment_stopped == RUNNING && current->virt_start_time != NOTSET && current->freeze_time == 0)
	{	spin_unlock(&current->dialation_lock);
        	list_for_each_safe(pos, n, &exp_list)
        	{
                	task = list_entry(pos, struct dilation_task_struct, list);
			if (find_children_info(task->linux_task, current->pid) == 1) { // I think it checks if the curret task belongs to the list of tasks in the experiment (or their children)

				spin_lock(&current->dialation_lock);				
                now_new = get_dilated_time(current);
				dilTask = container_of(&current_task, struct dilation_task_struct, linux_task);

				if (flags & TIMER_ABSTIME)
					current->wakeup_time = timespec_to_ns(rqtp);
				else
					current->wakeup_time = now_new + ((rqtp->tv_sec*1000000000) + rqtp->tv_nsec)*Sim_time_scale; 

                sleep_helper = hmap_get(&task->sleep_process_lookup, &current->pid);
				if(sleep_helper == NULL){
					sleep_helper = kmalloc(sizeof(struct sleep_helper_struct), GFP_KERNEL);
					if(sleep_helper == NULL){
						printk(KERN_INFO "TimeKeeper : Sleep Process NOMEM");
						return -ENOMEM;
					}	
				}
				//set_current_state(TASK_INTERRUPTIBLE);
				init_waitqueue_head(&sleep_helper->w_queue);
				sleep_helper->done = 0;
				hmap_put(&task->sleep_process_lookup,&current->pid,sleep_helper);
				//current->freeze_time = now_new;

				//current->wakeup_time = now_new + ((rqtp->tv_sec*1000000000) + rqtp->tv_nsec)*Sim_time_scale;
				s64 temp_wakeup_time = current->wakeup_time;
				printk(KERN_INFO "TimeKeeper : PID : %d, New wake up time : %lld\n",current->pid, current->wakeup_time); 
				spin_unlock(&current->dialation_lock);

				if(sleep_helper->done == 0)
					wait_event(sleep_helper->w_queue,sleep_helper->done != 0);
				set_current_state(TASK_RUNNING);
				hmap_remove(&task->sleep_process_lookup, &current->pid);
				kfree(sleep_helper);
				

				//kill(current, SIGSTOP, dilTask); 
				return 0;
			} //end if
        	} //end for loop
	} //end if
	spin_unlock(&current->dialation_lock);

    return ref_sys_clock_nanosleep(which_clock, flags,rqtp, rmtp);

}

asmlinkage int sys_clock_gettime_new(const clockid_t which_clock, struct timespec __user * tp){

	struct list_head *pos;
	struct list_head *n;
	struct dilation_task_struct* task;
	struct dilation_task_struct *dilTask;
	struct timeval ktv;
	struct task_struct *current_task;
	s64 now;
	s64 now_new;
	s32 rem;
	s64 real_running_time;
	s64 dilated_running_time;
	current_task = current;
	int ret;
	struct timespec temp;
	s64 mono_time;

	struct timeval curr_tv;

	do_gettimeofday(&curr_tv);
	s64 undialated_time_ns = timeval_to_ns(&curr_tv);
	s64 boottime = 0;



	if(which_clock != CLOCK_REALTIME && which_clock != CLOCK_MONOTONIC && which_clock != CLOCK_MONOTONIC_RAW && which_clock != CLOCK_REALTIME_COARSE && which_clock != CLOCK_MONOTONIC_COARSE)
		return ref_sys_clock_gettime(which_clock,tp);


	ret = ref_sys_clock_gettime(which_clock,tp);
	if(copy_from_user(&temp,tp,sizeof(tp)))
		return -EFAULT;

	mono_time = timespec_to_ns(&temp);
	boottime = undialated_time_ns - mono_time;

	//return ret;

	


	spin_lock(&current->dialation_lock);
	if (experiment_stopped == RUNNING && current->virt_start_time != NOTSET && current->freeze_time == 0)
	{	spin_unlock(&current->dialation_lock);
        	list_for_each_safe(pos, n, &exp_list)
        	{
                	task = list_entry(pos, struct dilation_task_struct, list);
			if (find_children_info(task->linux_task, current->pid) == 1) { // I think it checks if the curret task belongs to the list of tasks in the experiment (or their children)
				now = get_dilated_time(current_task);
				now = now - boottime;
				struct timespec tempStruct = ns_to_timespec(now);
				if(copy_to_user(tp, &tempStruct, sizeof(tempStruct)))
					return -EFAULT;
				return 0;				

			}
		}
	}
	spin_unlock(&current->dialation_lock);

	return ref_sys_clock_gettime(which_clock,tp);



}






