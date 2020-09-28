Installation
============

Minimum System Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Kronos has been tested on Ubuntu 16.04.5 LTS. It uses a modified linux kernel v4.4.50. The system should consist of an Intel i5 or later processor with atleast 4 cores and 8 GB of RAM for good performance. It is preferable to install Kronos inside a VM with Virtualized Intel-VTx and CPU performance counters. This is known to avoid display driver issues on newer laptops/machines.

.. figure:: images/vmware_virtual_machine_settings_virt.png
  :alt: VMware Virtual Machine Settings Screenshot
  :width: 80%
  :align: center
  
  Virtualization Settings required for virtual machine in VMware.

Python version 3.6 or higher should be the default on your system.

Installing Kronos
^^^^^^^^^^^^^^^^^

To get started on Kronos, please perform the following setup steps:

* Disable Transparent HugePages: (Add the following to /etc/rc.local to permanently disable them)::

    if test -f /sys/kernel/mm/transparent_hugepage/enabled; then
      echo never > /sys/kernel/mm/transparent_hugepage/enabled
    fi
    if test -f /sys/kernel/mm/transparent_hugepage/defrag; then
      echo never > /sys/kernel/mm/transparent_hugepage/defrag
    fi
* Ensure that /etc/rc.local has execute permissions::

    sudo chmod +x /etc/rc.local

* Clone Repository into /home/${user} directory. Checkout the master branch::

    git clone https://github.com/Vignesh2208/Kronos.git

* Compile and configure Kronos kernel patch::
 
    cd ~/Kronos 
    sudo make setup_kernel

.. note:: Over the course of kernel setup, a menu config would appear. 

  The following additional config steps should also be performed inside menuconfig:

  1. Under General setup 
		     -->  Append a local kernel version name. (e.g it could be "-ins-VT")
		     
		     .. figure:: images/kernel_config_local_version.png
  			:alt: Kernel Configuration Screenshot for Local Version
  			:width: 80%
  			:align: center
  
  #. Under kernel_hacking 
		     --> enable Collect kernel timers statistics
		     
		     .. figure:: images/kernel_config_kernel_timers.png
  			:alt: Kernel Configuration Screenshot for Kernel Timers
  			:width: 80%
  			:align: center
		     
  #. Under Processor types and features 
                     --> Transparent Huge Page support 
                                                      --> Transparent Huge Page support sysfs defaults should be set to always
						      
		     .. figure:: images/kernel_config_transparent_hugepage_support.png
  			:alt: Kernel Configuration Screenshot for Transparent Huge Page Support
  			:width: 80%
  			:align: center	      

* Reboot the machine and into the new kernel (identifiable by the appended local kernel version name in the previous step)

* Build and load Kronos module::
 
    cd ~/Kronos
    sudo make build load
    
Ready to use VM
^^^^^^^^^^^^^^^

Link to a ready to use Kronos VM (version 1.3) can be downloaded from `here <https://drive.google.com/drive/folders/1wwJ6tO7XaGj3C_8ZophNsmbc2vO-oxH4?usp=sharing>`_.
The VM also contains installations of two companion projects `OpenSCADA <http://github.com/Vignesh2208/OpenSCADA>`_ and `Melody <http://github.com/Vignesh2208/Melody>`_.
which use Kronos for cyber-physical system modelling.

Username: kronos
Password: kronos

Make sure Kronos is loaded after each reboot before running experiments::

  cd ~/Kronos
  sudo make load


Verifying Installation
----------------------

The following tests (optional) can be run to verify the Kronos installation:

* INS-SCHED specific test::
    
    cd ~/Kronos/src/tracer/tests
    sudo make run_repeatability_test

* Kronos integration tests::

    cd ~/Kronos/test
    sudo make run

All of the above tests should print a SUCCESS message.

Loading Kronos after each reboot
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Inorder to use Kronos, it must be loaded after each VM/machine reboot. It can be loaded with the following command::

  cd ~/Kronos
  sudo make load


Patching Kronos kernel after an update
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If a previously installed Kronos kernel needs to be updated with new changes in the git repo::

  cd ~/Kronos && git pull origin master
  sudo make patch_kernel # Follow same installation steps when prompted in menuconfig
