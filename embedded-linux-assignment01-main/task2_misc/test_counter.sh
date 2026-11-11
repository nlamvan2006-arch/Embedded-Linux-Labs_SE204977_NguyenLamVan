#!/bin/sh

set -eu

echo "=== TASK 2: COUNTER TEST ==="

[ -c /dev/counter ] || {
    echo "FAIL: /dev/counter missing"
    exit 1
}

[ -r /proc/counter_info ] || {
    echo "FAIL: /proc/counter_info missing"
    exit 1
}

echo 10 > /dev/counter

a=$(cat /dev/counter)
b=$(cat /dev/counter)

echo "First read: $a"
echo "Second read: $b"

[ "$a" = "10" ] || exit 1
[ "$b" = "11" ] || exit 1

if echo -1 > /dev/counter 2>/dev/null; then
    echo "FAIL: negative value accepted"
    exit 1
fi

if echo abc > /dev/counter 2>/dev/null; then
    echo "FAIL: invalid text accepted"
    exit 1
fi

echo "=== PROCFS ==="
cat /proc/counter_info

echo "[PASS] Task 2 tests completed"
