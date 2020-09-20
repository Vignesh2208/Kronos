TEST_DIR=$(HOME)/Kronos/src/tracer/tests/thread-test
RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod
PYTHON:=python

.PHONY: clean
clean:
	@$(RM) -f ${TEST_DIR}/cmds.txt > /dev/null
	@$(RM) -f ${TEST_DIR}/thread_test > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Thread Test ..."
	@$(GCC) -w ${TEST_DIR}/thread_test.c -o ${TEST_DIR}/thread_test -lpthread
	@$(ECHO) ${TEST_DIR}/thread_test > ${TEST_DIR}/cmds.txt
	@$(CHMOD) 777 ${TEST_DIR}/thread_test

.PHONY: run
run:
	@$(ECHO) "Starting Thread Test ..."
	@$(PYTHON) $(HOME)/Kronos/examples/example_vt_experiment.py --cmds_to_run_file=${TEST_DIR}/cmds.txt
	@$(ECHO) "Thread Test Finished ..."

.PHONY: run_python
run_python:
	@$(ECHO) "Starting Thread Test ..."
	@$(ECHO) "python ${TEST_DIR}/thread.py" > ${TEST_DIR}/cmds.txt
	@$(PYTHON) $(HOME)/Kronos/examples/example_vt_experiment.py --cmds_to_run_file=${TEST_DIR}/cmds.txt
	@$(ECHO) "Thread Test Finished ..."

.PHONY: start
start: run

start_python: run_python
