import sys
import os
import time
import kronos_functions as kf
import sys
import argparse
import threading
import logging

EXP_CBE = 1
EXP_CS = 2

def start_new_dilated_process(timeline_id, cmd_to_run, log_file_fd,
    exp_type):
    
    newpid = os.fork()
    if newpid == 0:
        os.dup2(log_file_fd, sys.stdout.fileno())
        os.dup2(log_file_fd, sys.stderr.fileno())
        args = ["tracer", "-t", str(timeline_id),
                "-c", cmd_to_run]
        os.execvp(args[0], args)
    else:
        return newpid


def worker_thread_fn(progress_ns_per_round, num_progress_rounds, timeline_id):
    logging.info("Running Timeline: %d for %d rounds ... ", timeline_id,
            num_progress_rounds)

    start_time = float(time.time())
    if num_progress_rounds > 0 :
        num_finished_rounds = 0
        while num_finished_rounds < num_progress_rounds:
            kf.progressTimelineBy(timeline_id, progress_ns_per_round)
            num_finished_rounds += 1
            
    elapsed_time = float(time.time()) - start_time
    logging.info("Timeline: %d finished. Total time elapsed (secs) = %f",
        timeline_id, elapsed_time)
    

def main():

    
    parser = argparse.ArgumentParser()


    parser.add_argument('--cmds_to_run_file', dest='cmds_to_run_file',
                        help='path to file containing commands to run', \
                        type=str, default='cmds_to_run_file.txt')


    parser.add_argument('--progress_ns_per_round', dest='progress_ns_per_round',
                        help='Number of insns per round', type=int,
                        default=1000000)

    parser.add_argument('--num_progress_rounds', dest='num_progress_rounds',
                        help='Number of rounds to run', type=int,
                        default=1000)

    parser.add_argument('--exp_type', dest='exp_type',
        help='Number of rounds to run', type=int, default=EXP_CS)

    parser.add_argument('--num_timelines', dest='num_timelines',
                    help='Number of timelines in CS experiment', type=int,
                    default=1)

    args = parser.parse_args()
    log_fds = []
    tracer_pids = []
    cmds_to_run = []

    worker_threads = []

    logging.basicConfig(level=logging.INFO)
    
    if not os.path.isfile(args.cmds_to_run_file):
        logging.info("Commands file path is incorrect !")
        sys.exit(0)
    fd1 = open(args.cmds_to_run_file, "r")
    cmds_to_run = [x.strip() for x in fd1.readlines()]
    fd1.close()
    for i in range(0, len(cmds_to_run)) :
        with open("/tmp/tracer_log_%d.txt" %(i), "w") as f:
            pass
    log_fds = [ os.open("/tmp/tracer_log_%d.txt" %(i), os.O_RDWR | os.O_CREAT ) \
        for i in range(0, len(cmds_to_run)) ]
    num_tracers = len(cmds_to_run)
    num_timelines = len(cmds_to_run)
    #num_timelines = args.num_timelines

    input('Press any key to continue !')
    for i in range(0, num_tracers) :
        with open("/tmp/tracer_log_%d.txt" %(i), "w") as f:
            pass
    log_fds = [ os.open("/tmp/tracer_log_%d.txt" %(i), os.O_RDWR | os.O_CREAT ) \
        for i in range(0, num_tracers) ]

    logging.info("Initializing VT Module !") 
    if kf.initializeVTExp(args.exp_type, num_timelines, num_tracers) < 0 :
        logging.info("VT module initialization failed ! Make sure you are running\
               the dilated kernel and kronos module is loaded !")
        sys.exit(0)

    input('Press any key to continue !')
    
    logging.info("Starting all commands to run !")
    
    for i in range(0, num_tracers):
        logging.info("Starting tracer: %d", i + 1)
        start_new_dilated_process(i % num_timelines,
                                  cmds_to_run[i], log_fds[i], args.exp_type)
    input('Press any key to continue !')
    
    logging.info("Synchronizing and freezing tracers ...")
    while kf.synchronizeAndFreeze() <= 0:
        logging.info("VT Module >> Synchronize and Freeze failed."
                     " Retrying in 1 sec")
        time.sleep(1)

    input('Press any key to continue !')

    for index in range(num_timelines):
        logging.info("Main    : create and start thread %d.", index)
        x = threading.Thread(target=worker_thread_fn,
            args=(args.progress_ns_per_round, args.num_progress_rounds, index))
        worker_threads.append(x)
        x.start()

    for index, thread in enumerate(worker_threads):
        logging.info("Main    : before joining thread %d.", index)
        thread.join()
        logging.info("Main    : thread %d done", index)


    input("Press Enter to continue...")
    logging.info("Stopping Synchronized Experiment !")
    kf.stopExp()

    for fd in log_fds:
        os.close(fd)

    logging.info("Finished ! Logs of each ith tracer can be found "
                 "in /tmp/tracer_log_i.txt")
    
            

if __name__ == "__main__":
	main()
