home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
benchmarking_dir=${tracer_dir}/tests/benchmarking
clean:

	@rm -f ${benchmarking_dir}/cmds.txt > /dev/null
	@rm -f ${benchmarking_dir}/benchmark > /dev/null
	@rm -f ${benchmarking_dir}/test > /dev/null


build:
	@echo "Building Benchmarking Suite ..."
	@gcc -w -I/usr/local/include/perf -I/usr/local/lib/perf/include \
	-I${tracer_dir}/utils ${benchmarking_dir}/benchmark.c \
	${tracer_dir}/utils/linkedlist.c ${tracer_dir}/utils/hashmap.c \
	-o ${benchmarking_dir}/benchmark -lperf -lm
	@gcc ${benchmarking_dir}/test.c -o ${benchmarking_dir}/test
	@echo ${benchmarking_dir}/test > cmds.txt
	@chmod 777 ${benchmarking_dir}/test
	@chmod 777 ${benchmarking_dir}/benchmark

run:
	${benchmarking_dir}/benchmark ${benchmarking_dir}/cmds.txt

