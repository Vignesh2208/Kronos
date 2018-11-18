home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests

build:
	@cd ${test_dir}/repeatability-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/sleep-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/socket-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/vfork-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/thread-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/repeatability-test && $(MAKE) clean && $(MAKE) build;
	@cd ${test_dir}/benchmarking && $(MAKE) clean && $(MAKE) build;

run_repeatability_test:
	@cd ${test_dir}/repeatability-test && $(MAKE) run

run_sleep_test:
	@cd ${test_dir}/sleep-test && $(MAKE) start

run_socket_test:
	@cd ${test_dir}/socket-test && $(MAKE) start

run_vfork_test:
	@cd ${test_dir}/vfork-test && $(MAKE) start

run_thread_test:
	@cd ${test_dir}/thread-test && $(MAKE) start


clean:
	@cd ${test_dir}/repeatability-test && $(MAKE) clean 
	@cd ${test_dir}/sleep-test && $(MAKE) clean
	@cd ${test_dir}/vfork-test && $(MAKE) clean
	@cd ${test_dir}/thread-test && $(MAKE) clean
	@cd ${test_dir}/socket-test && $(MAKE) clean
	@cd ${test_dir}/benchmarking && $(MAKE) clean

