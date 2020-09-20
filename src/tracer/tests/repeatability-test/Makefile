TEST_DIR=$(HOME)/Kronos/src/tracer/tests/repeatability-test
TRACER_DIR=$(HOME)/Kronos/src/tracer
RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod

.PHONY: clean
clean:

	@$(RM) -f ${TEST_DIR}/cmds.txt > /dev/null
	@$(RM) -f ${TEST_DIR}/insn_tester > /dev/null
	@$(RM) -f ${TEST_DIR}/test > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Repeatability Test ..."
	@$(GCC) -w -I/usr/local/include/perf -I/usr/local/lib/perf/include \
	-I${TRACER_DIR}/utils ${TEST_DIR}/insn_tester.c ${TRACER_DIR}/utils/linkedlist.c \
	${TRACER_DIR}/utils/hashmap.c -o ${TEST_DIR}/insn_tester -lperf
	@$(GCC) ${TEST_DIR}/test.c -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -o ${TEST_DIR}/test
	@$(GCC) ${TEST_DIR}/test.c -S -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -o ${TEST_DIR}/test.s
	@$(ECHO) ${TEST_DIR}/test > cmds.txt
	@$(CHMOD) 777 ${TEST_DIR}/test
	@$(CHMOD) 777 ${TEST_DIR}/insn_tester

.PHONY: run
run:
	@$(ECHO) Starting Repeatability Test ...
	@$(ECHO) 0 | sudo tee /proc/sys/kernel/randomize_va_space
	${TEST_DIR}/insn_tester ${TEST_DIR}/cmds.txt
	@$(ECHO) 2 | sudo tee /proc/sys/kernel/randomize_va_space
	@$(ECHO) Repeatability Test Finished ...

