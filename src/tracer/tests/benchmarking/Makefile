TRACER_DIR=$(HOME)/Kronos/src/tracer
BENCHMARKING_DIR=${TRACER_DIR}/tests/benchmarking

RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod

.PHONY: clean
clean:

	@$(RM) -f ${BENCHMARKING_DIR}/cmds.txt > /dev/null
	@$(RM) -f ${BENCHMARKING_DIR}/benchmark > /dev/null
	@$(RM) -f ${BENCHMARKING_DIR}/test > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Benchmarking Suite ..."
	@$(GCC) -w -I/usr/local/include/perf -I/usr/local/lib/perf/include \
	-I${TRACER_DIR}/utils ${BENCHMARKING_DIR}/benchmark.c \
	${TRACER_DIR}/utils/linkedlist.c ${TRACER_DIR}/utils/hashmap.c \
	-o ${BENCHMARKING_DIR}/benchmark -lperf -lm
	@$(GCC) ${BENCHMARKING_DIR}/test.c -o ${BENCHMARKING_DIR}/test
	@$(ECHO) ${BENCHMARKING_DIR}/test > cmds.txt
	@$(CHMOD) 777 ${BENCHMARKING_DIR}/test
	@$(CHMOD) 777 ${BENCHMARKING_DIR}/benchmark

.PHONY: run
run:
	${BENCHMARKING_DIR}/benchmark ${BENCHMARKING_DIR}/cmds.txt

