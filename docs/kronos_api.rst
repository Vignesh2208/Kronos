Kronos API
==========


In this section we briefly describe python and C APIs provided for invoking Kronos specific functions. These API calls need to be invoked by the central orchestrator script.
It assumes the default python version is 3.6 or higher.

Kronos Python API
^^^^^^^^^^^^^^^^^

The initialization process also installs a python module called kronos_functions. The orchestrator script needs to import this module. For the rest of this discussion, we use an example python orchestrator script included with the installation. It can be found `here <https://github.com/Vignesh2208/Kronos/tree/master/examples/example_kronos_experiment.py>`_::

	import kronos_functions as kf

Initializing Kronos
-------------------

To initialize Kronos the **initializeExp(num_expected_tracers)** API call must be made. It takes in as input the number of tracers that will be launched subsequently::

	if kf.initializeExp(num_expected_tracers) < 0 :
		print ("Kronos initialization failed ! Make sure you are running the dilated kernel and kronos module is loaded !")
		sys.exit(0)

Synchronize and Freeze
----------------------

The Synchronize and Freeze API can be invoked::

	while kf.synchronizeAndFreeze() <= 0:
		print ("Kronos >> Synchronize and Freeze failed. Retrying in 1 sec")
		time.sleep(1)

Progress for specifed number of rounds
--------------------------------------

To run the experiment for a specified number of rounds the **progressBy(duration_ns, num_rounds)** API call is used::

	num_finished_rounds = 0
        step_size = 100
        while num_finised_rounds <= 10000:
            kf.progressBy(1000000, step_size)
            num_finished_rounds += step_size
            print ("Ran %d rounds ..." %(num_finished_rounds))

In this example the experiment is run in bursts of 100 rounds before returning control to the orchestrator script. During
each round, virtual time advances by 1000000 ns or 1ms.


Stop Experiment
---------------

To stop the dilated experiment, **stopExp()** API call is invoked::

	kf.stopExp()

Kronos C API
^^^^^^^^^^^^

An almost identical set of API calls are provided in C as well. An orchestrator script which uses C API must include the Kronos_functions.h header file::

	#include "Kronos_functions.h"

	The function prototypes of all the relevant API calls are given below::

	//! Initializes a EXP_CBE experiment with specified number of expected tracers
	int initializeExp(int num_expected_tracers);

	//! Synchronizes and Freezes all started tracers
	int synchronizeAndFreeze(void);

	//! Initiates Stoppage of the experiment
	int stopExp(void);

	//! Instructs the experiment to be run for the specific number of rounds 
	//  where each round signals advancement of virtual time by the specified duration in nanoseconds
	int progressBy(s64 duration, int num_rounds);


The orchestrator script must be linked with the kronos api library with **-lkronosapi** linker option at compile time. This library gets included in the system path when Kronos is first installed.


