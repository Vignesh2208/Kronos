Kronos controlled execution
===========================

Linux processes can be added to Kronos's control with API calls. Once a process is added to Kronos's schedule queue, its execution order and duration is controlled by Kronos. We refer to these processes as dilated processes. All new threads and processes which may be spawned by any exiting dilated process is automatically detected and added to Kronos's control.

In this section, we briefly discuss the stages involved in dilating a process and subsequently advancing all dilated processes in virtual synchrony. For each stage, either APIs or helper binaries are provided.

In any Kronos controlled execution, a central orchestrator script is written by the user. The orchestrator script should accept as input a set of processes to dilate. These processes are expressed as linux commands with arguments. It should then invoke kronos specific APIs or helper binaries for each stage. All the involved stages are listed in the correct order below:

* **Initializing Kronos**: The first step in building a Kronos controlled emulation involves initializing Kronos itself.  Python and C APIs are provided for this purpose. This stage assumes that Kronos module is already build and loaded (Refer installation section).

* **Start Dilated Processes**: In this stage, processes which need to be dilated are passed as arguments to helper binaries provided by Kronos called **tracers**. A tracer binary is incharge of launching and monitoring each dilated process assigned to it. It also registers each started dilated process with Kronos. The orchestrator script may fork and launch one or more tracers.

* **Synchronize and Freeze**: After all **tracers** have been launched, the next stage involves waiting for all of them to finish initialization. An API call is provided for this purpose. After the completing of this stage, all dilated processes have been added to Kronos's control and their execution and clock has been frozen.

* **Experiment Progress**: This stage involves progressing/running the experiment. The experiment proceeds in rounds. During each round, all dilated processes are advanced by a timestep. At the end of each round, the clocks of all dilated processes are synchronized to the same value and all dilated processes are frozen. APIs are provided to specify number of rounds to run.

* **Stop Experiment**: The final stage involves invoking a Stop Experiment API call after the desired number of rounds have been run. This cleans up all allocated resources and stops all tracers.

In subsequent sections, we describe each stage in more detail.
