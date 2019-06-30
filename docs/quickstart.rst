Quickstart Example
==================

In this section we discuss a simple example which ties all of the previously described steps before. It uses an example python orchestrator script included with the installation. It can be found `here <https://github.com/Vignesh2208/Kronos/tree/master/examples/example_kronos_experiment.py>`_. The rest of this section assumes that Kronos has already been loaded.

Commands/processes which need to be dilated can be added with arguments to the `cmds_to_run_file.txt <https://github.com/Vignesh2208/Kronos/tree/master/examples/cmds_to_run_file.txt>`_ file.

The example can be started as follows::

	python example_kronos_experiment.py --cmds_to_run_file=$HOME/Kronos/examples/cmds_to_run_file.txt \
					    --run_in_one_tracer=False \
					    --rel_cpu_speed=1.0 \
					    --num_insns_per_round=1000000 \
					    --num_progress_rounds=1000

.. note:: The run_in_one_tracer argument specifies if all commands in the cmd file need to launched under just one tracer. If specified to False, a new tracer is launched for each command.

This example performs all the initialization steps and runs a virtual time synchronized experiment. The stdout/stderr of each dilated process can be found inside its corresponding tracer_log located inside the **/tmp** directory.
	
					    
