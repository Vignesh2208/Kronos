home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests/repeatability-test

clean:

	@rm -f ${test_dir}/cmds.txt > /dev/null
	@rm -f ${test_dir}/insn_tester > /dev/null
	@rm -f ${test_dir}/test > /dev/null


build:
	@echo "Building Repeatability Test ..."
	@gcc -w -I/usr/local/include/perf -I/usr/local/lib/perf/include \
	-I${tracer_dir}/utils ${test_dir}/insn_tester.c ${tracer_dir}/utils/linkedlist.c \
	${tracer_dir}/utils/hashmap.c -o ${test_dir}/insn_tester -lperf
	@gcc ${test_dir}/test.c -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -o ${test_dir}/test
	@echo ${test_dir}/test > cmds.txt
	@chmod 777 ${test_dir}/test
	@chmod 777 ${test_dir}/insn_tester

run:
	@echo Starting Repeatability Test ...
	@echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
	${test_dir}/insn_tester ${test_dir}/cmds.txt
	@echo 2 | sudo tee /proc/sys/kernel/randomize_va_space
	@echo Repeatability Test Finished ...

