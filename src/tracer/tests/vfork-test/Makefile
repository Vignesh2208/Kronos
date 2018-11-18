home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests/vfork-test

clean:
	@rm -f ${test_dir}/cmds.txt > /dev/null
	@rm -f ${test_dir}/vfork_test > /dev/null

build:
	@echo "Building Vfork Test ..."
	@gcc -w ${test_dir}/vfork_test.c -o ${test_dir}/vfork_test
	@echo ${test_dir}/vfork_test > ${test_dir}/cmds.txt
	chmod 777 ${test_dir}/vfork_test

run:
	@echo "Starting Vfork Test ..."
	tracer_test -f ${test_dir}/cmds.txt
	@echo "VFork test Finished ..."

start: run
