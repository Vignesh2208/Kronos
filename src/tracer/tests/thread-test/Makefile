home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests/thread-test

clean:
	@rm -f ${test_dir}/cmds.txt > /dev/null
	@rm -f ${test_dir}/thread_test > /dev/null


build:
	@echo "Building Thread Test ..."
	@gcc -w ${test_dir}/thread_test.c -o ${test_dir}/thread_test -lpthread
	@echo ${test_dir}/thread_test > ${test_dir}/cmds.txt
	chmod 777 ${test_dir}/thread_test

run:
	@echo "Starting Thread Test ..."
	tracer_test -f ${test_dir}/cmds.txt
	@echo "Thread Test Finished ..."

run_python:
	@echo "Starting Thread Test ..."
	@echo "python thread.py" > ${test_dir}/cmds.txt
	tracer_test -f ${test_dir}/cmds.txt
	@echo "Thread Test Finished ..."


start: run

start_python: run_python