
TEST_DIR=$(HOME)/Kronos/src/tracer/tests/sleep-test
RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod
PYTHON:=python

.PHONY: clean
clean:
	@$(RM) -f ${TEST_DIR}/cmds.txt > /dev/null
	@$(RM) -f ${TEST_DIR}/test > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Sleep Test ..."
	@$(GCC) -w ${TEST_DIR}/test.c -o ${TEST_DIR}/test 
	@$(CHMOD) 777 ${TEST_DIR}/test

.PHONY: run
run:
	@$(ECHO) "Starting Sleep Test ..."
	@$(ECHO) ${TEST_DIR}/test > ${TEST_DIR}/cmds.txt
	@$(PYTHON) $(HOME)/Kronos/examples/example_vt_experiment.py --cmds_to_run_file=${TEST_DIR}/cmds.txt
	@$(ECHO) "Sleep Test Finished ..."

.PHONY: start
start: run
