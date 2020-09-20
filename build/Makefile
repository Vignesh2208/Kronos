EXTRA_CFLAGS += 
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
KBUILD_CFLAGS += -w
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules

obj-m:= vt_module.o
vt_module-objs := ../src/core/vt_module.o ../src/core/common.o ../src/core/sync_experiment.o ../src/core/utils.o ../src/utils/hashmap.o ../src/utils/linkedlist.o

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR)/build modules 

clean:
	$(RM) -f *.ko *.o *.mod.c Module.symvers .*test_module* > /dev/null
	$(RM) -rf .tmp_versions
