TEST_DIR=$(HOME)/Kronos/src/tracer/tests/socket-test
RM:=rm
ECHO:=echo
GCC:=gcc
CHMOD:=chmod
PYTHON:=python

.PHONY: clean
clean:
	@$(RM) -f ${TEST_DIR}/cmds.txt > /dev/null

.PHONY: build
build:
	@$(ECHO) "Building Socket Test ..."
	@$(ECHO) "python ${TEST_DIR}/udp_server.py\npython ${TEST_DIR}/udp_client.py" > ${TEST_DIR}/cmds.txt

.PHONY: run
run:
	@$(ECHO) "Starting Socket test ..."
	@$(PYTHON) $(HOME)/Kronos/examples/example_vt_experiment.py --cmds_to_run_file=${TEST_DIR}/cmds.txt
	@$(ECHO) "Socket Test Finished ..."

.PHONY: start
start: run
