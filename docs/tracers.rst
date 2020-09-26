Tracers
=======

Tracers are helper binaries that ship with Kronos installation. They are added to the system path and can be invoked with the following arguments::

	tracer [-f <Path to file containg commands to dilate> or -c <Command to dilate with args> ] \
		-r <Relative CPU Speed> [-i <Optional Tracer-ID> ]

A brief description of each option is given below:

* **-r** : Relative CPU speed is equivalent to a time dilation factor or (TDF). It represents the number of instructions that need to be executed for 1ns of virtual time.

* **-i** : Represents an optional ID assigned to a tracer. If absent the ID will be auto assigned. Note: Either ignore this for all tracers or specify a unique value (starting from 1) for each tracer. Do not partially assign values form some tracers. 

* **-f** : Specifies a path to file containg commands to dilate. If more than one dilated process needs to be monitored by this tracer, then the commands to dilate are put in a file and passed as an argument. 

* **-c** : Specifies a single command which needs to be dilated. Both **-f** and **-c** cannot be used simultaneously.

For example, a tracer which wants to dilate the bash command **ls -al** can be launched as follows::

	tracer -c "/bin/ls -al" -r 1.0
 
A tracer will launch all dilated processes and register itself with Kronos. It will then wait for trigger commands from Kronos. At the start of each round, Kronos would send a trigger to each registered tracer instructing it to run one round.

The tracer could be incharge of multiple processes because multiple processes were assigned to it from the start or the originally launched process spawned new threads. In such cases, the tracer would perform round-robin fair scheduling among all process and threads in its control. Each process is assigned a fixed quanta instruction burst size of **min(100000, num_insns_per_round)**. After a dilated process executes of the assigned quanta, the clock of all processes under the relevant tracer is increased by a **timestep size** computed from the number of executed instructions and relative cpu speed.
