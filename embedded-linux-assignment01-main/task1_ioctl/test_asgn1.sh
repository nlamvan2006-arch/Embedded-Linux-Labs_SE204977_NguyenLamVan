#!/bin/sh

echo "================================"
echo " Assignment 01 - Task 1 Test"
echo "================================"

if [ ! -c /dev/asgn1 ]; then
    echo "FAIL: /dev/asgn1 not found"
    exit 1
fi

if [ ! -x /usr/bin/test_asgn1 ]; then
    echo "FAIL: /usr/bin/test_asgn1 not found"
    exit 1
fi

echo "[TEST] Running ioctl tests..."

if /usr/bin/test_asgn1; then
    echo "[PASS] Task 1 ioctl tests completed"
else
    echo "[FAIL] Task 1 ioctl tests failed"
    exit 1
fi
