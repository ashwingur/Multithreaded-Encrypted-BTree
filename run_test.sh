#!/bin/bash

count=0 # number of test cases run so far

# Note that the longtimetable test may take a few seconds to complete.
echo "---Compiling test cases---"
for test in testcases/*.c; do
    name=$(basename $test .c)
    # Compile the file
    gcc -O0 -Werror=vla -std=gnu11 -g -fsanitize=address -pthread -lrt -o testcases/$name $test libbtreestore.a -lm 
    echo "$name.c compiled"
done
echo ""
echo "---Running tests---"
for test in testcases/*.c; do
    name=$(basename $test .c)
    expected=testcases/$name.out
    echo "Running test $name"
    ./testcases/$name | diff - $expected || echo "Test $name: failed!"
    count=$((count+1))
done

echo "Finished running $count tests!"