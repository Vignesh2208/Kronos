home_dir=$(HOME)
tracer_dir=${home_dir}/Kronos/src/tracer
test_dir=${tracer_dir}/tests/socket-test

clean:
	@rm -f ${test_dir}/cmds.txt > /dev/null

build:
	@echo "Building Socket Test ..."
	@echo "python udp_server.py\npython udp_client.py" > ${test_dir}/cmds.txt

run:
	@echo "Starting Socket test ..."
	tracer_test -f ${test_dir}/cmds.txt
	@echo "Socket Test Finished ..."

start: run
