#!/bin/bash

sudo rm -f /tmp/tracer*
sudo mkdir -p /tmp/tfd_test
sudo run_test 1 1 10000 1000000 > /tmp/tfd_test/tfd_test.log
sudo cp /tmp/tracer* /tmp/tfd_test 2> /dev/null
if grep -nr "Succeeded" /tmp/tfd_test/tfd_test.log; then
	echo "STATUS: COMPLETED. Check Logs inside /tmp/tfd_test"
else
	echo "STATUS: FAILED. Check Logs at /tmp/tfd_test"
fi
