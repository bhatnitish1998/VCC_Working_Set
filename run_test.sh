#!/bin/bash
make

mkdir -p logs
# ./working_set <sample_interval> <sample_size> <random_percent> <workload file> <estimation method>

# access bit method vs page invalidation method performance
# performance for different sample size
./working_set 3 100 100 workload1.txt 0 > logs/am_100.txt
./working_set 3 1000 100 workload1.txt 0 > logs/am_1000.txt
./working_set 3 10000 100 workload1.txt 0 > logs/am_10000.txt

echo "Access method performance test completed"

./working_set 3 100 100 workload1.txt 1 > logs/im_100.txt
./working_set 3 1000 100 workload1.txt 1 > logs/im_1000.txt
./working_set 3 10000 100 workload1.txt 1 > logs/im_10000.txt

echo "Invalidation performance test completed"

# sampling size accuracy when memory access pattern are randomized.
./working_set 3 100 0 workload1.txt 1 > logs/r_0_100.txt
./working_set 3 1000 0 workload1.txt 1 > logs/r_0_1000.txt
./working_set 3 10000 0 workload1.txt 1 > logs/r_0_10000.txt

echo "Random access 0% completed "

./working_set 3 100 20 workload1.txt 1 > logs/r_20_100.txt
./working_set 3 1000 20 workload1.txt 1 > logs/r_20_1000.txt
./working_set 3 10000 20 workload1.txt 1 > logs/r_20_10000.txt

echo "Random access 20% completed "

./working_set 3 100 100 workload1.txt 1 > logs/r_100_100.txt
./working_set 3 1000 100 workload1.txt 1 > logs/r_100_1000.txt
./working_set 3 10000 100 workload1.txt 1 > logs/r_100_10000.txt

echo "Random access 100% completed "

# sampling rate for different workload.
./working_set 1 1000 100 workload1.txt 1 > logs/im_1_w1.txt
./working_set 1 1000 100 workload2.txt 1 > logs/im_1_w2.txt

echo "Sample interval 1 second completed"

./working_set 3 1000 100 workload1.txt 1 > logs/im_3_w1.txt
./working_set 3 1000 100 workload2.txt 1 > logs/im_3_w2.txt

echo "Sample interval 3 second completed"

./working_set 5 1000 100 workload1.txt 1 > logs/im_5_w1.txt
./working_set 5 1000 100 workload2.txt 1 > logs/im_5_w2.txt

echo "Sample interval 5 second completed"

./working_set 10 1000 100 workload1.txt 1 > logs/im_10_w1.txt
./working_set 10 1000 100 workload2.txt 1 > logs/im_10_w2.txt

echo "Sample interval 10 second completed"
