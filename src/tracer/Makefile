
home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
EXTRA_CFLAGS += 
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(shell pwd)
GCC:=gcc
RM:=rm

libs:
	@cd ${tracer_dir}/libperf  && ./configure  && $(MAKE) clean && $(MAKE) && $(MAKE) install
	

build_tracer:
	 @gcc -w -I/usr/local/include/perf -I/usr/local/lib/perf/include -I${tracer_dir}/utils \
	 ${tracer_dir}/tracer.c ${tracer_dir}/utils.c ${tracer_dir}/utils/linkedlist.c \
	 ${tracer_dir}/utils/hashmap.c -o tracer -lperf -lkronosapi
	 @gcc  -w ${tracer_dir}/tests/cmd_sender.c -o cmd_sender
	 @chmod 777 tracer
	 @chmod 777 cmd_sender
	 @mv tracer /usr/bin
	 @mv cmd_sender /usr/bin
	 @cp /usr/local/lib/libperf.so.0 /usr/lib/
	 @cd ${tracer_dir}/utils && $(MAKE) build

build_tracer_debug:
	@gcc -w -I/usr/local/include/perf -I/usr/local/lib/perf/include -I${tracer_dir}/utils \
	-D DEBUG ${tracer_dir}/tracer.c ${tracer_dir}/utils.c ${tracer_dir}/utils/linkedlist.c \
	${tracer_dir}/utils/hashmap.c -o tracer -lperf -lkronosapi
	@gcc  -w ${tracer_dir}/tests/cmd_sender.c -o cmd_sender
	@chmod 777 tracer
	@chmod 777 cmd_sender
	@mv tracer /usr/bin
	@mv cmd_sender /usr/bins
	@cp /usr/local/lib/libperf.so.0 /usr/lib/
	@cd ${tracer_dir}/utils && $(MAKE) build

build_tracer_test: 
	@gcc -w -I/usr/local/include/perf -I/usr/local/lib/perf/include -I${tracer_dir}/utils \
	-D TEST -D DEBUG_VERBOSE ${tracer_dir}/tracer.c ${tracer_dir}/utils.c ${tracer_dir}/utils/linkedlist.c \
	${tracer_dir}/utils/hashmap.c -o tracer_test -lperf -lkronosapi
	@gcc  -w ${tracer_dir}/tests/cmd_sender.c -o cmd_sender
	@chmod 777 tracer_test
	@chmod 777 cmd_sender
	@mv tracer_test /usr/bin
	@mv cmd_sender /usr/bin
	@cp /usr/local/lib/libperf.so.0 /usr/lib/
	@cd ${tracer_dir}/utils && $(MAKE) build

clean: 
	@cd ${tracer_dir}/libperf && ./configure && $(MAKE) clean
	@cd ${tracer_dir}/tests && $(MAKE) clean
	@rm -f *.o > /dev/null
	@rm -f /usr/bin/tracer > /dev/null
	@rm -f /usr/bin/tracer_test > /dev/null
	@rm -f /usr/bin/cmd_sender > /dev/null
	@cd ${tracer_dir}/utils && $(MAKE) clean

build_tests:
	@cd ${tracer_dir}/tests && $(MAKE) build
	@cd ${tracer_dir}/utils && $(MAKE) build

build: libs build_tracer build_tracer_test build_tests

debug: libs build_tracer_test build_tracer_debug build_tests








