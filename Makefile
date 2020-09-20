EXTRA_CFLAGS += 
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(shell pwd)
GCC:=gcc
RM:=rm
CD:=cd
ECHO:=echo

nCpus=$(shell nproc --all)

all: clean build

.PHONY : clean
clean: clean_core clean_utils clean_api clean_tracer clean_scripts

.PHONY : build
build: build_core build_api build_tracer build_scripts

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

.PHONY : clean_tracer
clean_tracer:
	@$(ECHO) "Cleaning Tracer files ..."
	@$(RM) -f src/tracer/*.o src/tracer/utils/*.o src/tracer/tests/*.o
	@$(CD) src/tracer && $(MAKE) clean
