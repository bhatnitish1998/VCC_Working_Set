#!/bin/bash
make

mkdir -p logs
# ./working_set <sample_interval> <sample_size> <random_percent> <workload file> <estimation method>

# ------------------Performance Test--------------------------
echo "Progress 0/10"
# normal
./working_set 50 10000 100 workload_high.txt 0 > logs/no_high.txt

echo "Progress 1/10"

# different sampling size high workload
./working_set 1 100 100 workload_high.txt 0 > logs/am_100_high.txt
./working_set 1 1000 100 workload_high.txt 0 > logs/am_1000_high.txt
./working_set 1 10000 100 workload_high.txt 0 > logs/am_10000_high.txt

echo "Progress 2/10"

./working_set 1 100 100 workload_high.txt 1 > logs/im_100_high.txt
./working_set 1 1000 100 workload_high.txt 1 > logs/im_1000_high.txt
./working_set 1 10000 100 workload_high.txt 1 > logs/im_10000_high.txt

echo "Progress 3/10"

# different sampling size low workload
./working_set 50 10000 100 workload_low.txt 0 > logs/no_low.txt

./working_set 1 100 100 workload_low.txt 0 > logs/am_100_low.txt
./working_set 1 1000 100 workload_low.txt 0 > logs/am_1000_low.txt
./working_set 1 10000 100 workload_low.txt 0 > logs/am_10000_low.txt

echo "Progress 4/10"

./working_set 1 100 100 workload_low.txt 1 > logs/im_100_low.txt
./working_set 1 1000 100 workload_low.txt 1 > logs/im_1000_low.txt
./working_set 1 10000 100 workload_low.txt 1 > logs/im_10000_low.txt

echo "Progress 5/10"
# ------------------Accuracy Test--------------------------

# sample size with random memory access
./working_set 1 100 0 workload_normal.txt 1 > logs/r_0_100.txt
./working_set 1 1000 0 workload_normal.txt 1 > logs/r_0_1000.txt
./working_set 1 10000 0 workload_normal.txt 1 > logs/r_0_10000.txt

echo "Progress 6/10"

./working_set 1 100 20 workload_normal.txt 1 > logs/r_20_100.txt
./working_set 1 1000 20 workload_normal.txt 1 > logs/r_20_1000.txt
./working_set 1 10000 20 workload_normal.txt 1 > logs/r_20_10000.txt

echo "Progress 7/10"

./working_set 1 100 100 workload_normal.txt 1 > logs/r_100_100.txt
./working_set 1 1000 100 workload_normal.txt 1 > logs/r_100_1000.txt
./working_set 1 10000 100 workload_normal.txt 1 > logs/r_100_10000.txt

echo "Progress 8/10"

# sampling rate for different workload.
./working_set 1 10000 100 workload_normal.txt 1 > logs/im_1_wn.txt
./working_set 2 10000 100 workload_normal.txt 1 > logs/im_2_wn.txt
./working_set 5 10000 100 workload_normal.txt 1 > logs/im_5_wn.txt

echo "Progress 9/10"
./working_set 1 10000 100 workload_rapid.txt 1 > logs/im_1_wr.txt
./working_set 2 10000 100 workload_rapid.txt 1 > logs/im_2_wr.txt
./working_set 5 10000 100 workload_rapid.txt 1 > logs/im_5_wr.txt

echo "Progress 10/10"
