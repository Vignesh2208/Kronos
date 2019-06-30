About
=====

Network emulators enable rapid prototyping and testing of applications. In a typical emulation the execution order and process execution burst lengths are managed by the host plat-form’s operating system, largely independent of the emulator. Timer based mechanisms are typically used, but the imprecision of timer firings introduces imprecision in the advancement of time. This imprecision leads to statistical variation in behavior which is not due to the model.

Kronos is a small set of modifications to the Linux kernel that uses precise instruction level tracking of process execution and control over execution order of con-
tainers, and so improve the mapping of executed behavior to advancement in time. This, and control of execution and placement of emulated processes in virtual time make the behavior of the emulation independent of the CPU resources of the platform which hosts the emulation. Under Kronos each process has its own virtual clock which is advanced based on a count of the number of x86 assembly instructions executed by its children. This mechanism is called INS-SCHED or instruction driven scheduling and it uses hardware performance counters underneath. 

With INS-SCHED, Kronos can precisely control execution order and duration of each emulated process which makes the overall emulation more repeatable and less susceptible to interference from the underlying operating system.
