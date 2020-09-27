TRACER_DIR=$(HOME)/Kronos/src/tracer
EXTRA_CFLAGS += 
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(shell pwd)
GCC:=gcc
RM:=rm
CD:=cd
CHMOD:=chmod
MV:=mv
CP:=cp


.PHONY: libs
libs:
	@$(CD) ${TRACER_DIR}/libperf  && ./configure  && $(MAKE) clean && $(MAKE) && $(MAKE) install
	

.PHONY: build_tracer
build_tracer:
	@$(GCC) -w -Wno-incompatible-pointer-types -I/usr/local/include/perf -I/usr/local/lib/perf/include -I${TRACER_DIR}/utils \
	 ${TRACER_DIR}/tracer.c ${TRACER_DIR}/utils.c ${TRACER_DIR}/utils/linkedlist.c \
	 ${TRACER_DIR}/utils/hashmap.c -o tracer -lperf -lkronosapi
	@$(CHMOD) 777 tracer
	@$(MV) tracer /usr/bin
	@$(GCC) ${TRACER_DIR}/app_vt_tracer.c -o app_vt_tracer -lpthread -lkronosapi
	@$(MV) app_vt_tracer /usr/bin
	@$(CP) /usr/local/lib/libperf.so.0 /usr/lib/
	@$(CD) ${TRACER_DIR}/utils && $(MAKE) build

.PHONY: build_tracer_debug
build_tracer_debug:
	@$(GCC) -w -Wno-incompatible-pointer-types -I/usr/local/include/perf -I/usr/local/lib/perf/include -I${TRACER_DIR}/utils \
	-D DEBUG ${TRACER_DIR}/tracer.c ${TRACER_DIR}/utils.c ${TRACER_DIR}/utils/linkedlist.c \
	${TRACER_DIR}/utils/hashmap.c -o tracer -lperf -lkronosapi
	@$(GCC) ${TRACER_DIR}/app_vt_tracer.c -o app_vt_tracer -lpthread -lkronosapi
	@$(MV) app_vt_tracer /usr/bin
	@$(CHMOD) 777 tracer
	@$(MV) tracer /usr/bin
	@$(CP) /usr/local/lib/libperf.so.0 /usr/lib/
	@$(CD) ${TRACER_DIR}/utils && $(MAKE) build

.PHONY: clean
clean: 
	@$(CD) ${TRACER_DIR}/libperf && ./configure && $(MAKE) clean
	@$(CD) ${TRACER_DIR}/tests && $(MAKE) clean
	@$(RM) -f *.o > /dev/null
	@$(RM) -f /usr/bin/tracer > /dev/null
	@$(RM) -f /usr/bin/tracer_test > /dev/null
	@$(RM) -f /usr/bin/app_vt_tracer > /dev/null
	@$(CD) ${TRACER_DIR}/utils && $(MAKE) clean

.PHONY: build_tests
build_tests:
	@$(CD) ${TRACER_DIR}/tests && $(MAKE) build
	@$(CD) ${TRACER_DIR}/utils && $(MAKE) build

.PHONY: build
build: libs build_tracer build_tests

.PHONY: debug
debug: libs build_tracer_test build_tracer_debug build_tests








