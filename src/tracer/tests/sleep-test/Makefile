home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests/sleep-test

clean:
	@rm -f ${test_dir}/cmds.txt > /dev/null
	@rm -f ${test_dir}/test > /dev/null

build:
	@echo "Building Sleep Test ..."
	@gcc -w ${test_dir}/test.c -o ${test_dir}/test 
	chmod 777 ${test_dir}/test

run:
	@echo "Starting Sleep Test ..."
	@echo ${test_dir}/test > ${test_dir}/cmds.txt
	tracer_test -f ${test_dir}/cmds.txt

	@echo "Sleep Test Finished ..."


start: run
