/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string> 
#include <climits>
#include <memory>
#include <unordered_map>

#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>  // for open
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <unistd.h>  // for close



#include <stdint.h>
#include <sys/ioctl.h>

extern "C" {    
  #include "cJSON.h"
}

#include "pin.H"

#define MAX_API_ARGUMENT_SIZE 1000
#define BUF_MAX_SIZE MAX_API_ARGUMENT_SIZE
#define VT_ADD_TO_SQ 'a'
#define VT_WRITE_RESULTS 'b'

#define ROB 1024
#define PMODEL_CPU_CLOCK_SPEED_GHZ 2.7


typedef long long s64;

typedef struct ioctl_args_struct {
  char cmd_buf[BUF_MAX_SIZE];
  s64 cmd_value;
} ioctl_args;

#define VT_ADD_TO_SQ 'a'
#define VT_WRITE_RES 'b'



using std::ostream;
using std::cout;
using std::cerr;
using std::string;
using std::endl;
using std::unordered_map;
//using std::unique_ptr;

//unordered_map<std::string, float> insnTimings;


#include <fcntl.h>  // for open
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <unistd.h>  // for close

const char *FILENAME = "/proc/kronos";

void flushBuffer(char *buf, int size) {
  if (size) memset(buf, 0, size * sizeof(char));
}

struct MachineInsPrefixTreeNode {
   char component;    
   float avgCpuCycles;
   unordered_map<char, MachineInsPrefixTreeNode *> children;
};

class MachineInsPrefixTree {
    private:
      MachineInsPrefixTreeNode * root;

    public:
      float avgArchInsnCycles;
      int totalArchInsns;
      MachineInsPrefixTree() {
      root = new struct MachineInsPrefixTreeNode();
            root->component = '\0';
            root->avgCpuCycles = -1;
            avgArchInsnCycles = 0;
            totalArchInsns = 0;
      };

      void addMachineInsn(std::string insnName, float avgCpuCycles) {
        struct MachineInsPrefixTreeNode * curr_node = root;
        MachineInsPrefixTreeNode * tmp;
        avgArchInsnCycles += avgCpuCycles;
        totalArchInsns ++;
        for (unsigned i=0; i< insnName.length(); ++i) {
          auto got = curr_node->children.find(insnName.at(i));
          if (got == curr_node->children.end()) {
              tmp =  new struct MachineInsPrefixTreeNode();
              tmp->component = insnName.at(i);
              tmp->avgCpuCycles = -1;

              if (i == insnName.length() - 1)
                tmp->avgCpuCycles = avgCpuCycles;
              curr_node->children.insert(std::make_pair(tmp->component, tmp));
              curr_node = curr_node->children.find(insnName.at(i))->second;
          } else {
              curr_node = 
                (struct MachineInsPrefixTreeNode *)got->second;
              if ((i == insnName.length() - 1))
            curr_node->avgCpuCycles = avgCpuCycles;
          }
        }

      };
      float getAvgCpuCycles(std::string insnName) {
		    struct MachineInsPrefixTreeNode * curr_node = root;
	      for (unsigned i=0; i< insnName.length(); ++i) {
		      auto got = curr_node->children.find(insnName.at(i));
		      if (got == curr_node->children.end())
		        return std::max((float)1.0, curr_node->avgCpuCycles); // longest-prefix match
		      curr_node = (struct MachineInsPrefixTreeNode *)got->second;
		      if ((i == insnName.length() - 1))
		        return std::max((float)1.0, curr_node->avgCpuCycles);
	      }
	      return 1.0;
      };

      void clearRecurse(struct MachineInsPrefixTreeNode * curr_node) {
	      if (!curr_node)
		      return;
	      for (auto it = curr_node->children.begin();
          it != curr_node->children.end(); it++) {
	 	      clearRecurse(it->second);
        }
        curr_node->children.clear();
	      delete curr_node;
      }

      void clean() {
	      struct MachineInsPrefixTreeNode * curr_node = root;
	      for (auto it = curr_node->children.begin();
            it != curr_node->children.end(); it++) {
	 	      clearRecurse(it->second);
        }
        delete root;
      }
};


MachineInsPrefixTree machineInsPrefixTree;


/*
Sends a specific command to the Kronos Kernel Module.
To communicate with the LKM, you send messages to the location
specified by FILENAME
*/
s64 sendToVtModule(ioctl_args *arg) {
  int fp = open(FILENAME, O_RDWR);
  int ret = 0;
  if (fp < 0) {
    printf("ERROR communicating with VT module: Could not open proc file\n");
    return -1;
  }

  if (!arg) {
     printf("ERROR communicating with VT module: Missing argument !\n");
     close(fp);
     return -1;
  }
  if (strlen(arg->cmd_buf) > BUF_MAX_SIZE) {
     printf("ERROR communicating with VT module: Too much data to copy !\n");
     close(fp);
     return -1;
  }
  ret = write(fp, arg->cmd_buf, strlen(arg->cmd_buf));
  arg->cmd_value = (s64) ret;
  close(fp);
  return ret;
}

void initIoctlArg(ioctl_args *arg) {
  if (arg) {
    flushBuffer(arg->cmd_buf, BUF_MAX_SIZE);
    arg->cmd_value = 0;
  }
}

int numCharacters(int n) {
  int count = 0;
  if (n < 0) {
    count = 1;
    n = -1 * n;
  }
  while (n != 0) {
    n /= 10;
    ++count;
  }
  return count;
}

int appendToIoctlArg(ioctl_args *arg, int *append_values, int num_values) {
  if (!arg) return -1;

  for (int i = 0; i < num_values; i++) {
    if (strlen(arg->cmd_buf) + numCharacters(append_values[i]) + 1 >=
        BUF_MAX_SIZE)
      return -1;

    if (i < num_values - 1)
      sprintf(arg->cmd_buf + strlen(arg->cmd_buf), "%d,", append_values[i]);
    else {
      sprintf(arg->cmd_buf + strlen(arg->cmd_buf), "%d", append_values[i]);
    }
  }
  return 0;
}



s64 writeTracerResults(int tracer_id, int* results, int num_results) {
  if (tracer_id < 0 || num_results < 0) {
    printf("Write tracer results: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  initIoctlArg(&arg);

  if (num_results == 0)
    sprintf(arg.cmd_buf, "%c,%d", VT_WRITE_RES, tracer_id);
  else {
    sprintf(arg.cmd_buf, "%c,%d,", VT_WRITE_RES, tracer_id);
    if (appendToIoctlArg(&arg, results, num_results) < 0) return -1;
  }

  return sendToVtModule(&arg);
}


int addProcessesToTracerSq(int tracer_id, int* pids, int num_pids) {
  if (tracer_id < 0 || num_pids <= 0) {
    printf("addProcessesToTracerSq: incorrect parameters for tracer: %d\n",
          tracer_id);
    return -1;
  }
  ioctl_args arg;
  initIoctlArg(&arg);
  sprintf(arg.cmd_buf, "%c,%d,", VT_ADD_TO_SQ, tracer_id);
  if (appendToIoctlArg(&arg, pids, num_pids) < 0) return -1;

  return sendToVtModule(&arg);
}


KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "", "specify output file name");

KNOB<int> KnobTotalNumTracers(KNOB_MODE_WRITEONCE, "pintool",
    "n", "0", "specify total number of tracers in experiment");

KNOB<int> KnobMonitorTracerId(KNOB_MODE_WRITEONCE, "pintool",
    "i", "0", "specify id of monitoring tracer");

KNOB<float> KnobRelCpuSpeed(KNOB_MODE_WRITEONCE, "pintool",
    "r", "1.0", "rel_cpu_speed");


#ifndef DISABLE_VT
INT32 totalNumTracers;
INT32 monitorTracerId;

float relCpuSpeed = 1.0;

s64 * tracerClockArray = NULL;
s64 * myClock = NULL;

INT32 numThreads = 0;
ostream* OutFile = NULL;


// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 39  // 64 byte line size: 64-25

// a running count of the instructions
class thread_data_t
{
  public:
    thread_data_t() : _increment(0), _curr_clock(0), _target_clock(0), _vt_enabled(0), _fp(0) {}
    UINT64 _increment;
    UINT64 _curr_clock;
    UINT64 _target_clock;
    UINT8 _vt_enabled;
    INT32 _fp;
    UINT8 _buf[10];
};

// key for accessing TLS storage in the threads. initialized once in main()
static  TLS_KEY tls_key = INVALID_TLS_KEY;
#endif

BOOL FollowChild(CHILD_PROCESS childProcess, VOID * userData) {
    fprintf(stdout, "before child:%u\n", getpid());
    return TRUE;
}     


// This function is called before every block
VOID PIN_FAST_ANALYSIS_CALL docount(UINT32 c, THREADID threadid) {
    #ifndef DISABLE_VT
    thread_data_t* tdata = static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, threadid));
    if (*myClock >= (s64)tdata->_target_clock || tdata->_curr_clock >= tdata->_target_clock) {
      if (tdata->_vt_enabled) {
        // stop here
        //tdata->_increment = writeTracerResults(monitorTracerId, NULL, 0);
        if (tdata->_fp < 0) {
            printf("Opening communication with VT module\n");
            tdata->_fp = open(FILENAME, O_RDWR);
            if (tdata->_fp < 0 ) {
              printf("ERROR Opening communication with VT module\n");
              PIN_ExitApplication(0);
            }
            sprintf((char *)tdata->_buf, "%c,%d",  VT_WRITE_RES, (int)monitorTracerId);
        }
        tdata->_increment = write(tdata->_fp, 
          (char *)tdata->_buf, strlen((char *)tdata->_buf));
        
        
        if (tdata->_increment <= 0) {
          tdata->_vt_enabled = 0;
          cout << "Trigerring Application Exit: TID:" << PIN_GetTid() << endl;
          PIN_ExitApplication(0);
        } else {
          tdata->_curr_clock = *myClock;
          // multiplied by rel-cpu-speed
          //printf ("Increment = %ld\n", (long)tdata->_increment);
          tdata->_target_clock = tdata->_curr_clock + (UINT64)(tdata->_increment * relCpuSpeed);
        }

      }
  } 
  tdata->_curr_clock += c;
  #endif
    
}


VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {
    #ifndef DISABLE_VT
    int myPid;
    numThreads++;
    thread_data_t* tdata = new thread_data_t;
    if (PIN_SetThreadData(tls_key, tdata, threadid) == FALSE) {
        cerr << "PIN_SetThreadData failed" << endl;
        PIN_ExitProcess(1);
    }

    cout << "Calling ThreadStart for thread: "
         << threadid << ", TID: " << PIN_GetTid() << endl;
    fflush(stdout);
    myPid = PIN_GetTid();
    if (addProcessesToTracerSq(monitorTracerId, &myPid, 1) < 0) {
      cout << "Thread: "<< myPid << "  Add to TRACER SQ failed !" << endl;
      tdata->_vt_enabled = 0;
      tdata->_curr_clock = 0;
      tdata->_target_clock = 0;
      tdata->_fp = -1;
      memset((char *)tdata->_buf, 0, 10);
      
      PIN_ExitProcess(1);
    } else {
      tdata->_vt_enabled = 1;
      tdata->_curr_clock = *myClock;
      tdata->_target_clock = *myClock;
      tdata->_fp = -1;
      memset(tdata->_buf, 0, 10);
    }
    cout << "Calling ThreadStart for thread: " << threadid << ", TID: "
         << PIN_GetTid() << " Successfull !" << " FP = " << tdata->_fp << endl;
    fflush(stdout);
    #endif
}


// Pin calls this function every time a new basic block is encountered.
// It inserts a call to docount.
VOID Trace(TRACE trace, VOID *v) {
    // Visit every basic block  in the trace

    #ifdef EXPERIMENTAL_VT_PMODEL
    double totalMBBCycles = 0.0;
    #endif
    
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      // Insert a call to docount for every bbl, passing the number of instructions.  
      #ifdef EXPERIMENTAL_VT_PMODEL
      for(INS ins= BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
        double requestedInsCycle = 0.0;
        requestedInsCycle = machineInsPrefixTree.getAvgCpuCycles(INS_Mnemonic(ins));
        totalMBBCycles += requestedInsCycle;
      }
      double minVal = std::min((int)BBL_NumIns(bbl), ROB);
      totalMBBCycles = totalMBBCycles / sqrt(minVal);
      if (totalMBBCycles < 1.0)
        totalMBBCycles = 1.0;
      BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)docount, IARG_FAST_ANALYSIS_CALL,
                      IARG_UINT32, (int)totalMBBCycles, IARG_THREAD_ID, IARG_END);
      #else
      BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)docount, IARG_FAST_ANALYSIS_CALL,
                    IARG_UINT32, BBL_NumIns(bbl), IARG_THREAD_ID, IARG_END);
      #endif
    }
}


VOID AfterForkInChild(THREADID threadid, const CONTEXT* ctxt, VOID * arg) {
    #ifndef DISABLE_VT
    int myPid;
    thread_data_t* tdata = static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, threadid));
    cout << "Calling ForkStart for thread: " << threadid << " PID: " << PIN_GetPid() << endl;
    myPid = PIN_GetPid();
    if (addProcessesToTracerSq(monitorTracerId, &myPid, 1) < 0) {
      tdata->_vt_enabled = 0;
      tdata->_curr_clock = 0;
      tdata->_target_clock = 0;
      tdata->_fp = -1;
      memset((char *)tdata->_buf, 0, 10);
      cout << "Process: "<< myPid << "  Add to TRACER SQ failed !" << endl;
      PIN_ExitProcess(1);
    } else {
      tdata->_vt_enabled = 1;
      tdata->_curr_clock = *myClock;
      tdata->_target_clock = *myClock;
      tdata->_fp = -1;
      memset((char *)tdata->_buf, 0, 10);
    }
    #endif
    
}


// This function is called when the thread exits
VOID ThreadFini(THREADID threadIndex, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    #ifndef DISABLE_VT
    int myPid, ret;
    thread_data_t* tdata = static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, threadIndex));
    myPid = PIN_GetTid();
    cout << "Calling ThreadFini for thread: "<< threadIndex << " , PID: " << PIN_GetTid() << endl;
    if (tdata->_vt_enabled) {
      cout << "Removing from tracer schedule queue ... " << endl;
      ret = writeTracerResults(monitorTracerId, &myPid, 1);
      cout << "Remove from tracer queue. Ret = " << ret << endl;
    }
    close(tdata->_fp);

    cout << "ThreadFini successfull ..." << threadIndex << endl;    
    delete tdata;
    #endif
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {

    #ifndef DISABLE_VT
    int num_pages, page_size;
    page_size = sysconf(_SC_PAGE_SIZE);
    num_pages = (totalNumTracers * sizeof(s64))/page_size;
    num_pages ++;


    *OutFile << "Finishing ! Total number of threads = " << numThreads << endl;
    munmap(tracerClockArray, page_size*num_pages);
    tracerClockArray = NULL;
    myClock = NULL;
    #endif
    machineInsPrefixTree.clean();
    
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
    cerr << "This tool manages virtual time driven execution" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return 1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{

    // Initialize pin

    
    printf ("Starting pin program !\n");
    fflush(stdout);

    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return Usage();
 
    #ifndef DISABLE_VT
    int num_pages, page_size, fd;
    page_size = sysconf(_SC_PAGE_SIZE);
    OutFile = KnobOutputFile.Value().empty() ? &cout : new std::ofstream(KnobOutputFile.Value().c_str());

    if (KnobTotalNumTracers.Value() == 0) {
      cerr << "Total Number of Tracers not specified !" << endl;
      return Usage();
    }

    totalNumTracers = KnobTotalNumTracers.Value();

    if (KnobMonitorTracerId.Value() == 0) {
      cerr << "Monitoring tracer-id not specified !" << endl;
      return Usage();
    }

    monitorTracerId = KnobMonitorTracerId.Value();

    if (monitorTracerId <= 0 || totalNumTracers <= 0 || monitorTracerId > totalNumTracers) {
      cerr << "Incorrect arguments to pinTool !" << endl;
      return Usage();
    }

    relCpuSpeed = KnobRelCpuSpeed.Value();
    if (relCpuSpeed < 0)
	    relCpuSpeed = 1.0;

    #ifdef EXPERIMENTAL_VT_PMODEL
    relCpuSpeed = PMODEL_CPU_CLOCK_SPEED_GHZ; // Hard coded 2.7 Ghz processor for now.
    #endif
    num_pages = (totalNumTracers * sizeof(s64))/page_size;
    num_pages ++;
    
    fd = open("/proc/kronos", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        fflush(stdout);
        return 1;
    }
    cout << "fd = " << fd << " attempting to MMAP: " << num_pages << " pages" << endl;
    puts("mmaping /proc/kronos");
    tracerClockArray = (s64 *)mmap(NULL, page_size*num_pages,
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (tracerClockArray == MAP_FAILED) {
        perror("mmap failed !");
        return 1;
    }

    close(fd);
    myClock = (s64 *)&tracerClockArray[monitorTracerId - 1];
    

    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(NULL);
    if (tls_key == INVALID_TLS_KEY)
    {
        cerr << "number of already allocated keys reached the MAX_CLIENT_TLS_KEYS limit" << endl;
        PIN_ExitProcess(1);
    }
    #endif
    

    printf("Adding instrumentation functions\n");
    
    #ifdef EXPERIMENTAL_VT_PMODEL
    char * file_content = 0;
    int length;
    cJSON *insnTimingsJson;
    cJSON *avgLatencyJson;
    cJSON *insnNameJson;
    int counter = 0;
    FILE * f = fopen ("/home/kronos/Kronos/src/tracer/pintool/timings/skylake.json", "r");
    printf("Parsing Insn Timing file ...");
    if (f) {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      file_content = (char *)malloc (length);
      if (file_content) fread (file_content, 1, length, f);
      fclose (f);
    } else {
	    printf ("File is empty !\n");
    }

    if (!length || !file_content) {
      printf("Insn Timings file empty! Ignoring ...\n");
      return 0;
    }

    insnTimingsJson = cJSON_Parse(file_content);
    if (!insnTimingsJson) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
        fprintf(stderr, "Insn Timings file parse error: %s. Ignoring ...\n", error_ptr);
      exit(0);
    }
    insnNameJson = cJSON_GetObjectItemCaseSensitive(insnTimingsJson, "insn_name");
    avgLatencyJson = cJSON_GetObjectItemCaseSensitive(insnTimingsJson, "avg_latency");
    char counterString[10];
    while (1) {
      memset(counterString, 0, 10);
      sprintf(counterString, "%d", counter);
            cJSON * insn = cJSON_GetObjectItemCaseSensitive(insnNameJson, counterString);
            cJSON * timing = cJSON_GetObjectItemCaseSensitive(avgLatencyJson, counterString);
      if (!insn)
        break;
      std::string insnName = std::string(cJSON_GetStringValue(insn));
      float insnTiming = cJSON_GetNumberValue(timing);
            machineInsPrefixTree.addMachineInsn(insnName, insnTiming);
      counter ++;
    }

    free(file_content);
    #endif
    
    PIN_AddFollowChildProcessFunction(FollowChild, 0);

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, NULL);

    // Register Fini to be called when thread exits.
    PIN_AddThreadFiniFunction(ThreadFini, NULL);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, NULL);

    PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, AfterForkInChild, 0);

    // Register Instruction to be called to instrument instructions.
    TRACE_AddInstrumentFunction(Trace, NULL);

    // Start the program, never returns
    fflush(stdout);
    PIN_StartProgram();

    return 1;
}
