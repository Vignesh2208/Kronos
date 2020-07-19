#!/bin/bash

sudo rm -f /tmp/tracer*
sudo mkdir -p /tmp/socket_test
sudo ./socket_test > /tmp/socket_test/socket_test.log
sudo cp /tmp/tracer* /tmp/socket_test 2> /dev/null
if grep -nr "Succeeded" /tmp/socket_test/socket_test.log; then
	echo "STATUS: COMPLETED. Check Logs inside /tmp/socket_test"
else
	echo "STATUS: FAILED. Check Logs inside /tmp/socket_test"
fi
