Kronos API
==========


In this section we briefly describe python and C APIs provided for invoking Kronos specific functions. These API calls need to be invoked by the central orchestrator script.

Kronos Python API
^^^^^^^^^^^^^^^^^

The initialization process also installs a python module called kronos_functions. The orchestrator script needs to import this module. For the rest of this discussion, we use an example python orchestrator script included with the installation. It can be found `here <https://github.com/Vignesh2208/Kronos/tree/master/examples/example_kronos_experiment.py>`_::

	import kronos_functions as kf

Initializing Kronos
-------------------

To initialize Kronos the **initializeExp(1)** API call must be made::

	if kf.initializeExp(1) < 0 :
		print "Kronos initialization failed ! Make sure you are running\
		    the dilated kernel and kronos module is loaded !"
		sys.exit(0)

Synchronize and Freeze
----------------------

The Synchronize and Freeze API takes in as input the number of launched tracers. It can be invoked as **synchronizeAndFreeze(num_tracers)**. For example::

	while kf.synchronizeAndFreeze(num_tracers) <= 0:
		print "Kronos >> Synchronize and Freeze failed. Retrying in 1 sec"
		time.sleep(1)

Start Experiment
----------------

Start Experiment can be triggered with the **startExp()** API call::

	kf.startExp()

Progress for specifed number of rounds
--------------------------------------

To run the experiment for a specified number of rounds the **progress_n_rounds(num_rounds)** API call is used::

	num_finised_rounds = 0
        step_size = 100
        while num_finised_rounds <= args.num_progress_rounds:
            kf.progress_n_rounds(step_size)
            num_finised_rounds += step_size
            print "Ran %d rounds ..." %(num_finised_rounds)


Stop Experiment
---------------

To stop the dilated experiment, **stopExp()** API call is invoked::

	kf.stopExp()

Kronos C API
^^^^^^^^^^^^

An almost identical set of API calls are provided in C as well. An orchestrator script which uses C API must include the Kronos_functions.h header file::

	#include "Kronos_functions.h"

The function prototypes of all the relevant API calls are given below::

	int initializeExp(int exp_type);
	int startExp();
	int synchronizeAndFreeze(int n_expected_tracers);
	int progress_n_rounds(int n_rounds);
	int stopExp();
	
.. note:: initializeExp takes an integer exp_type argument which must be set to 1. It is currently just a place holder but future upgrades are planned.

The orchestrator script must be linked with the kronos api library with **-lkronosapi** linker option at compile time. This library gets included in the system path when Kronos is first installed.


