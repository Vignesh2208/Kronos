EXTRA_CFLAGS += 
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(shell pwd)
GCC:=gcc
RM:=rm
CD:=cd
CP:=cp
ECHO:=echo
CHMOD:=chmod

nCpus=$(shell nproc --all)

all: clean build

.PHONY : clean
clean: clean_core clean_utils clean_api clean_tracer clean_scripts clean_pintool

.PHONY : build
build: build_core build_api build_tracer build_scripts build_pintool

.PHONY : setup_kernel
setup_kernel: download_4_4_kernel compile_4_4_kernel

.PHONY : patch_kernel
patch_kernel: update_kernel compile_4_4_kernel

.PHONY : update_kernel
update_kernel:
	@$(CD) src/kernel_changes/linux-4.4.5 && ./patch_kernel.sh

.PHONY : download_4_4_kernel
download_4_4_kernel:
	@$(CD) src/kernel_changes/linux-4.4.5 && ./setup.sh

.PHONY : compile_4_4_kernel
compile_4_4_kernel:
	@$(CD) /src/linux-4.4.5 && sudo cp -v /boot/config-`uname -r` .config && $(MAKE) menuconfig && $(MAKE) -j$(nCpus) && $(MAKE) modules_install && $(MAKE) install;

.PHONY : build_core
build_core: 
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR)/build modules

.PHONY : build_api
build_api:
	@$(CD) src/api; $(MAKE) build_api;

.PHONY : build_tracer
build_tracer:
	@$(CD) src/tracer; $(MAKE) build;

.PHONY : build_pintool
build_pintool:
	@$(ECHO) "Building pintool ..."
	@$(CD) src/tracer/pintool; $(MAKE) obj-intel64/inscount_tls.so TARGET=intel64
	@$(CP) src/tracer/pintool/pin-3.13/pin /usr/bin
	@$(CHMOD) 0777 /usr/bin/pin
	@$(CP) src/tracer/pintool/obj-intel64/inscount_tls.so /usr/lib/inscount_tls.so
	@$(CHMOD) 0777 /usr/lib/inscount_tls.so

.PHONY : build_scripts
build_scripts:
	@$(CD) scripts; $(MAKE) build;

.PHONY : load
load:
	sudo insmod build/vt_module.ko

.PHONY : unload
unload:
	sudo rmmod build/vt_module.ko

.PHONY : clean_scripts
clean_scripts:
	@$(ECHO) "Cleaning scripts ..."
	@$(CD) scripts && $(MAKE) clean
	
.PHONY : clean_core
clean_core:
	@$(ECHO) "Cleaning old build files ..."
	@$(RM) -f build/*.ko build/*.o build/*.mod.c build/Module.symvers build/modules.order ;

.PHONY : clean_utils
clean_utils:
	@$(ECHO) "Cleaning old utils files ..."
	@$(RM) -f src/utils/*.o 	

.PHONY : clean_api
clean_api:
	@$(ECHO) "Cleaning old api files ..."
	@$(RM) -f src/api/*.o 	
	@$(CD) src/api && $(MAKE) clean

.PHONY : clean_pintool
clean_pintool:
	@$(ECHO) "Cleaning pintool ..."
	$(RM) -rf src/tracer/pintool/obj-intel64 > /dev/null
	$(RM) -f /usr/bin/pin > /dev/null 
	$(RM) -f /usr/lib/inscount_tls.so > /dev/null

.PHONY : clean_tracer
clean_tracer:
	@$(ECHO) "Cleaning Tracer files ..."
	@$(RM) -f src/tracer/*.o src/tracer/utils/*.o src/tracer/tests/*.o
	@$(CD) src/tracer && $(MAKE) clean
