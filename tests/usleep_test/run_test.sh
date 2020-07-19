#!/bin/bash

sudo rm -f /tmp/tracer*
sudo mkdir -p /tmp/usleep_test
sudo ./usleep_test 1 > /tmp/usleep_test/usleep_test.log
sudo cp /tmp/tracer* /tmp/usleep_test 2> /dev/null
if grep -nr "Succeeded" /tmp/usleep_test/usleep_test.log; then
	echo "STATUS: COMPLETED. Check Logs inside /tmp/usleep_test"
else
	echo "STATUS: FAILED. Check Logs inside /tmp/usleep_test"
fi
