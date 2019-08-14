Tracked Bugs
============

The Kronos virtual system currenly some limitations which are currently tracked `here`_.  


.. _here: https://github.com/Vignesh2208/Kronos/blob/master/TODO

* Kronos is unable to trace/control bash/shell scripts. So if an application/process started under the control of Kronos tries to run shell/bash scripts, then Kronos would be unable to control that process or its children and their behaviour is undefined. This is because the underlying linux ptrace subsystem used by Kronos is designed for compiled programs and not shell scripts.

* Further, if a Kronos experiment is abnormally stopped/cancelled without a clean stop, then the next Kronos experiment cannot be run without a system reboot. This issue is being currently worked on.


