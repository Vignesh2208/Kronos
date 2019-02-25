
## KRONOS
```
Kronos is a linux virtual time system for scalable and repeatable Network emulation. Most notably, it uses a new technique
to control advancement of virtual time called INS-SCHED or Instruction driven scheduling. With INS-SCHED, virtual time will
advancement in an emulation will be controlled based on the number of instructions executed by a process. 

Please refer the TODO file for currently tracked bugs.
```

```
Setup Requirements:

* Ubuntu 16.04 (Has been Tested with Ubuntu 16.04.5)

* Disable Transparent HugePages: (Add the following to /etc/rc.local to permanently disable them)

if test -f /sys/kernel/mm/transparent_hugepage/enabled; then
   echo never > /sys/kernel/mm/transparent_hugepage/enabled
fi
if test -f /sys/kernel/mm/transparent_hugepage/defrag; then
   echo never > /sys/kernel/mm/transparent_hugepage/defrag
fi

* Ensure that /etc/rc.local has execute permissions.
```

```
Installation Instructions

* Clone Repository into /home/${username} directory. Checkout the master branch

* Setup Kernel
	@cd ~/Kronos
	@sudo make setup_kernel
	
	During the setup process do not allow kexec tools to handle kernel reboots.
	Over the course of kernel setup, a menu config would appear. 

	The following additional config steps should also be performed inside menuconfig:
		Under General setup -->
			Append a local kernel version name. For example it could be -ins-VT.
		Under kernel_hacking -->
			enable Collect kernel timers statistics
		Under Processor types and features -->
			Transparent Huge Page support -->
				Transparent Huge Page support sysfs defaults should be set to always

* Reboot machine and boot into new kernel

* Build and Install Kronos
	@cd ~/Kronos
	@sudo make clean
	@sudo make build load
```

```
Verifying installation

* Running tests:
	Repeatability Test
	
	@cd ~/Kronos/src/tracer/tests && sudo make run_repeatability_test

	Run All Other Tests
		-Alarm Test
		-Timerfd Test
		-Usleep Test
		-Socket Test
		-Producer Consumer Test

	@cd ~/Kronos/tests && sudo make run

* All tests should produce a TEST_SUCCEEDED orTEST_COMPLETED message
```
