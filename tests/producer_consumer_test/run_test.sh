#!/bin/bash

sudo rm -f /tmp/tracer*
mkdir -p /tmp/pc_test
sudo run_test 1 1 1000 1000000 > /tmp/pc_test/pc_test.log
sudo cp /tmp/tracer* /tmp/pc_test 2> /dev/null
if grep -nr "Succeeded" /tmp/pc_test/pc_test.log; then
	echo "STATUS: COMPLETED. Check Logs inside /tmp/pc_test"
else
	echo "STATUS: FAILED. Check Logs inside /tmp/pc_test"
fi
