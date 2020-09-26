import sys
import os
import time
import kronos_functions as kf
import sys
import argparse


def start_new_dilated_process(cmd_to_run, rel_cpu_speed, log_file_fd):
    newpid = os.fork()
    if newpid == 0:
        os.dup2(log_file_fd, sys.stdout.fileno())
        os.dup2(log_file_fd, sys.stderr.fileno())
        args = ["tracer", "-c", cmd_to_run, "-r", str(rel_cpu_speed)]
        os.execvp(args[0], args)
    else:
        return newpid

def start_all_cmds_in_one_tracer(cmds_to_run_file_path, rel_cpu_speed, log_file_fd):
    newpid = os.fork()
    if newpid == 0:
        os.dup2(log_file_fd, sys.stdout.fileno())
        os.dup2(log_file_fd, sys.stderr.fileno())
        args = ["tracer", "-f", cmds_to_run_file_path, "-r", str(rel_cpu_speed)]
        os.execvp(args[0], args)
    else:
        return newpid


def main():

    
    parser = argparse.ArgumentParser()

    parser.add_argument('--cmds_to_run_file', dest='cmds_to_run_file',
                        help='path to file containing commands to run', \
                        type=str, default='cmds_to_run_file.txt')

    parser.add_argument('--run_in_one_tracer', dest='run_in_one_tracer',
            help='True/False runs all commands in one tracer if True',
            default="False")

    parser.add_argument('--rel_cpu_speed', dest='rel_cpu_speed',
                        help='relative cpu speed', type=float, \
                        default=1.0)

    parser.add_argument('--num_insns_per_round', dest='num_insns_per_round',
                        help='Number of insns per round', type=int,
                        default=1000000)

    parser.add_argument('--num_progress_rounds', dest='num_progress_rounds',
                        help='Number of rounds to run', type=int,
                        default=1000)

    args = parser.parse_args()
    
    log_fds = []
    tracer_pids = []
    cmds_to_run = []

    input('Press any key to continue !')

    if args.run_in_one_tracer == "True":
        fd = os.open( "/tmp/tracer_log.txt", os.O_RDWR | os.O_CREAT )
        log_fds.append(fd)
        num_tracers = 1
    else:
        if not os.path.isfile(args.cmds_to_run_file):
            print ("Commands file path is incorrect !")
            sys.exit(0)
        fd1 = open(args.cmds_to_run_file, "r")
        cmds_to_run = [x.strip() for x in fd1.readlines()]
        fd1.close()
        for i in range(0, len(cmds_to_run)) :
            with open("/tmp/tracer_log%d.txt" %(i), "w") as f:
                pass
        log_fds = [ os.open("/tmp/tracer_log%d.txt" %(i), os.O_RDWR | os.O_CREAT ) \
            for i in range(0, len(cmds_to_run)) ]
        num_tracers = len(cmds_to_run)

    print ("Initializing VT Module !" )  
    if kf.initializeExp(num_tracers) < 0 :
        print ("VT module initialization failed ! Make sure you are running\
                the dilated kernel and kronos module is loaded !")
        sys.exit(0)

    input('Press any key to continue !')
    
    print ("Starting all commands to run !")
    if args.run_in_one_tracer == "True":
        start_all_cmds_in_one_tracer(args.cmds_to_run_file, args.rel_cpu_speed, log_fds[0])
    else:
        for i in range(0, len(cmds_to_run)):
            print ("Starting tracer for cmd: %s" %(cmds_to_run[i]))
            start_new_dilated_process(cmds_to_run[i], args.rel_cpu_speed, log_fds[i])
    
    print ("Synchronizing anf freezing tracers ...")
    while kf.synchronizeAndFreeze() <= 0:
        print ("VT Module >> Synchronize and Freeze failed. Retrying in 1 sec")
        time.sleep(1)

    input('Press any key to continue !')


    print ("Starting Synchronized Experiment !")
    start_time = float(time.time())
    if args.num_progress_rounds > 0 :
        print ("Running for %d rounds ... " %(args.num_progress_rounds))

        num_finised_rounds = 0
        step_size = min(100, args.num_progress_rounds)
        while num_finised_rounds < args.num_progress_rounds:
            kf.progressBy(args.num_insns_per_round, step_size)
            num_finised_rounds += step_size
            print ("Ran %d rounds ..." %(num_finised_rounds), 
                   " elapsed time ...", float(time.time()) - start_time)

    elapsed_time = float(time.time()) - start_time
    print ("Total time elapsed (secs) = ", elapsed_time)
    input("Press Enter to continue...")
    print ("Stopping Synchronized Experiment !")
    kf.stopExp()

    for fd in log_fds:
        os.close(fd)

    print ("Finished ! Logs of each ith tracer can be found in /tmp/tracer_logi.txt")
    
            

if __name__ == "__main__":
	main()
