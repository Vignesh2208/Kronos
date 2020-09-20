TEST_DIR=$(HOME)/Kronos/src/tracer/tests/vfork-test
RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod
PYTHON:=python

.PHONY: clean
clean:
	@$(RM) -f ${TEST_DIR}/cmds.txt > /dev/null
	@$(RM) -f ${TEST_DIR}/vfork_test > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Vfork Test ..."
	@$(GCC) -w ${TEST_DIR}/vfork_test.c -o ${TEST_DIR}/vfork_test
	@$(ECHO) ${TEST_DIR}/vfork_test > ${TEST_DIR}/cmds.txt
	@$(CHMOD) 777 ${TEST_DIR}/vfork_test

.PHONY: run
run:
	@$(ECHO) "Starting Vfork Test ..."
	@$(PYTHON) $(HOME)/Kronos/examples/example_vt_experiment.py --cmds_to_run_file=${TEST_DIR}/cmds.txt
	@$(ECHO) "VFork test Finished ..."

.PHONY: start
start: run
