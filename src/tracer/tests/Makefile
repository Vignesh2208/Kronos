TEST_DIR=$(HOME)/Kronos/src/tracer/tests
CD:=cd

.PHONY: build
build:
	@$(CD) ${TEST_DIR}/repeatability-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/sleep-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/socket-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/vfork-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/thread-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/repeatability-test && $(MAKE) clean && $(MAKE) build;
	@$(CD) ${TEST_DIR}/benchmarking && $(MAKE) clean && $(MAKE) build;

.PHONY: run_repeatability_test
run_repeatability_test:
	@$(CD) ${TEST_DIR}/repeatability-test && $(MAKE) run

.PHONY: run_sleep_test
run_sleep_test:
	@$(CD) ${TEST_DIR}/sleep-test && $(MAKE) start

.PHONY: run_socket_test
run_socket_test:
	@$(CD) ${TEST_DIR}/socket-test && $(MAKE) start

.PHONY: run_vfork_test
run_vfork_test:
	@$(CD) ${TEST_DIR}/vfork-test && $(MAKE) start

.PHONY: run_thread_test
run_thread_test:
	@$(CD) ${TEST_DIR}/thread-test && $(MAKE) start

.PHONY: clean
clean:
	@$(CD) ${TEST_DIR}/repeatability-test && $(MAKE) clean 
	@$(CD) ${TEST_DIR}/sleep-test && $(MAKE) clean
	@$(CD) ${TEST_DIR}/vfork-test && $(MAKE) clean
	@$(CD) ${TEST_DIR}/thread-test && $(MAKE) clean
	@$(CD) ${TEST_DIR}/socket-test && $(MAKE) clean
	@$(CD) ${TEST_DIR}/benchmarking && $(MAKE) clean

