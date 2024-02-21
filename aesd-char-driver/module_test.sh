#!/bin/sh

echo "Starting module test script"

echo "---Cleaning"
make clean
echo "---Making"
make

if [ $? -ne 0 ]; then
    echo "Make failed"
    exit 1
fi

echo "---Unloading"
./aesdchar_unload
echo "---Loading"
./aesdchar_load

# strace -o /tmp/strace.txt cat /dev/aesdchar

if [ "$1" = "d" ]; then
    echo "---Running drivertest.sh"
    ../assignment-autotest/test/assignment8/drivertest.sh
fi
