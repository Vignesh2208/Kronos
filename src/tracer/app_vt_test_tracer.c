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

#define APP_VT_TEST

#define PTRACE_FORCE_SINGLESTEP 0x42f5

//#ifdef APP_VT_TEST
#include "Kronos_functions.h"
//#endif


#define NUM_THREADS 5
#define NUM_PROCESSES 1
 
int total_number_of_tracers;
int my_tracer_id;

s64 * tracer_clock_array = NULL;
s64 * my_clock = NULL;

void thread_exit() {
    printf("Alternate exit for thread !\n");
    exit(0);
}

int fibonacci(int max, int thread_id, s64 * thread_target_clock, s64 * thread_increment) {
   


   int r0 = 0;
   int r1 = 1;
   int sum = 0, i = 0;

   for (i = 1; i < max; i++) {
        sum = sum + r1 + r0;
        r0 = r1;
        r1 = i + 1;

        #ifdef APP_VT_TEST
        if (*thread_increment <= 0 || *my_clock >= *thread_target_clock) {
            *thread_increment = writeTracerResults(my_tracer_id, NULL, 0);
            if (*thread_increment <= 0)
                thread_exit();
            *thread_target_clock = *my_clock + *thread_increment;
        } else {
                *my_clock += 100;
                *thread_increment -= 100;
        }
        printf("Thread_id = %d, My clock = %llu, Increment = %llu\n",
               thread_id, *my_clock, *thread_increment);
        #endif
   }
   
   return sum;
}
// A normal C function that is executed as a thread  
// when its name is specified in pthread_create() 
void *myThreadFun(void *vargp) { 
    int *myid = (int *)vargp;
    int fib_value;
    int x = (int)syscall(__NR_gettid);
    int ret;
    s64 thread_increment, thread_target_clock;
    printf("Thread: %d, PID = %d\n", *myid, x);
    #ifdef APP_VT_TEST
    ret = addProcessesToTracerSq(my_tracer_id, &x, 1);
    if (ret < 0) {
        printf("Thread: %d Add to TRACER SQ failed !\n", *myid);
        return NULL;
    }
    printf("Thread: %d, added to  sq\n", *myid);
    fflush(stdout);
    #endif

    #ifdef APP_VT_TEST
    thread_increment = writeTracerResults(my_tracer_id, NULL, 0);
    if (thread_increment <= 0)
        thread_exit();
    thread_target_clock = *my_clock + thread_increment;
    #endif



    for (int i = 0; i < 100; i++) {
        printf("Starting Fibonacci from Thread: %d\n", *myid); 
        fib_value = fibonacci(10000, *myid, &thread_target_clock, &thread_increment);
        printf("Finished Fibonacci from Thread: %d. Value = %d\n", *myid, fib_value); 
        fflush(stdout);
        //sleep(1); 
        printf("Finished from Thread: %d\n", *myid); 
        fflush(stdout);
    }

    #ifdef APP_VT_TEST
    thread_increment = writeTracerResults(my_tracer_id, &x, 1);
    assert(thread_increment <= 0);
    #endif
    return NULL; 
} 

void ProcessFn(int id) {

    int *myid = (int *)&id;
    int fib_value;
    int x = (int)syscall(__NR_getpid);
    int ret;
    s64 thread_increment, thread_target_clock;
    printf("Process: %d, PID = %d\n", *myid, x);
    #ifdef APP_VT_TEST
    ret = addProcessesToTracerSq(my_tracer_id, &x, 1);
    if (ret < 0) {
        printf("Process: %d Add to TRACER SQ failed !\n", *myid);
        return NULL;
    }
    printf("Process: %d, added to  sq\n", *myid);
    fflush(stdout);
    #endif

    #ifdef APP_VT_TEST
    thread_increment = writeTracerResults(my_tracer_id, NULL, 0);
    if (thread_increment <= 0)
        thread_exit();
    thread_target_clock = *my_clock + thread_increment;
    #endif

    printf("Sleep start from Process: %d\n", *myid); 
    fflush(stdout);
    sleep(1); 
    printf("Sleep Finished from Process: %d\n", *myid); 
    fflush(stdout);

    for (int i = 0; i < 10; i++) {
        printf("Starting Fibonacci from Process: %d\n", *myid); 
        fib_value = fibonacci(10000, *myid, &thread_target_clock, &thread_increment);
        printf("Finished Fibonacci from Process: %d. Value = %d\n", *myid, fib_value); 
        fflush(stdout);
        sleep(1); 
        printf("Finished from Process: %d\n", *myid); 
        fflush(stdout);
    }

    #ifdef APP_VT_TEST
    thread_increment = writeTracerResults(my_tracer_id, &x, 1);
    assert(thread_increment <= 0);
    #endif
    return NULL; 
}
   
int main(int argc, char **argv) { 
    pthread_t thread_id[NUM_THREADS]; 
    int i;
    int id[NUM_THREADS];
    int pids[NUM_PROCESSES];
    int fd;
    int num_pages, ret, page_size, status;

    if (argc < 3) {
        printf("Usage: %s <total_num_tracers> <my_tracer_id>\n", argv[0]);
        return EXIT_FAILURE;
    }
    page_size = sysconf(_SC_PAGE_SIZE);


    total_number_of_tracers = atoi(argv[1]);
    my_tracer_id = atoi(argv[2]);

    if (my_tracer_id <= 0 || my_tracer_id > total_number_of_tracers) {
        printf("Incorrect arguments to APP-VT tracer\n");
        exit(-1);
    }

    #ifdef APP_VT_TEST

    num_pages = (total_number_of_tracers * sizeof(s64))/page_size;
    num_pages ++;
    

    ret = registerTracer(my_tracer_id, TRACER_TYPE_APP_VT, SIMPLE_REGISTRATION,
        0, 0);
    if (ret < 0) {
        printf("TracerID: %d, Registration Error. Errno: %d\n", my_tracer_id, errno);
        exit(-1);
    } else {
        printf("TracerID: %d, Assigned CPU: %d\n", my_tracer_id, ret);
    }


    fd = open("/proc/status", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        assert(0);
    }
    printf("fd = %d, attempting to MMAP: %d pages\n", fd, num_pages);
    puts("mmaping /proc/dilation/status");
    tracer_clock_array = (s64 *)mmap(NULL, page_size*num_pages, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (tracer_clock_array == MAP_FAILED) {
        perror("mmap failed !");
        assert(0);
    }
    my_clock = (s64 *)&tracer_clock_array[my_tracer_id - 1];
    #endif


    for (i = 0; i < NUM_THREADS - 1; i++) {
        id[i] = i;
        pthread_create(&thread_id[i], NULL, myThreadFun, (void *)&id[i]); 
    }

    for (i = 0; i < NUM_THREADS - 1; i++) {
        pthread_join(thread_id[i], NULL);
    } 

    for(i = 0; i < NUM_PROCESSES -1; i++) {
    pid_t pid = fork();
    if (pid > 0) {
        pids[i] = pid;
    } else {
        printf("Starting Process: %d\n", i);
        ProcessFn(i);
        exit(0);
    }
    }

    printf("Waiting for processes to finish !\n");
    for(i = 0; i < NUM_PROCESSES - 1; i++) {
          waitpid(pids[i], &status, 0);
    } 
    close(fd);
    exit(0); 
}
