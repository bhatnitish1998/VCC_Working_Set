#!/bin/bash
make

mkdir -p logs
# ./working_set <sample_interval> <sample_size> <random_percent> <workload file> <estimation method>

# ------------------Performance Test--------------------------
#echo "Progress 0/10"
## normal
#./working_set 50 1000 100 workload_low.txt 0 > logs/no_low.txt &
#./working_set 50 1000 100 workload_high.txt 0 > logs/no_high.txt &
#
## Access bit
#./working_set 1 1000 100 workload_low.txt 0 > logs/am_low.txt &
#./working_set 1 1000 100 workload_high.txt 0 > logs/am_high.txt &
#
## Page invalidation
#./working_set 1 1000 100 workload_low.txt 1 > logs/im_low.txt &
#./working_set 1 1000 100 workload_high.txt 1 > logs/im_high.txt &
#
#wait
#echo "Progress 1/10"
#
## different sample size
#./working_set 1 100 100 workload_high.txt 1 > logs/sz_100.txt &
#./working_set 1 500 100 workload_high.txt 1 > logs/sz_500.txt &
#./working_set 1 1000 100 workload_high.txt 1 > logs/sz_1000.txt &
#
## different sample rate
#./working_set 1 1000 100 workload_high.txt 1 > logs/sr_1.txt &
#./working_set 2 1000 100 workload_high.txt 1 > logs/sr_2.txt &
#./working_set 5 1000 100 workload_high.txt 1 > logs/sr_5.txt &
#
#wait

 #------------------Accuracy Test--------------------------

# sample size with random memory access
./working_set 1 100 0 workload_normal.txt 1 > logs/r_0_100.txt &
./working_set 1 500 0 workload_normal.txt 1 > logs/r_0_500.txt &
./working_set 1 1000 0 workload_normal.txt 1 > logs/r_0_1000.txt &

./working_set 1 100 100 workload_normal.txt 1 > logs/r_100_100.txt &
./working_set 1 500 100 workload_normal.txt 1 > logs/r_100_500.txt &
./working_set 1 1000 100 workload_normal.txt 1 > logs/r_100_1000.txt &

wait

echo "Progress 8/10"

# sampling rate for different workload.
./working_set 1 1000 100 workload_normal.txt 1 > logs/im_1_wn.txt &
./working_set 2 1000 100 workload_normal.txt 1 > logs/im_2_wn.txt &
./working_set 5 1000 100 workload_normal.txt 1 > logs/im_5_wn.txt &

echo "Progress 9/10"
./working_set 1 1000 100 workload_rapid.txt 1 > logs/im_1_wr.txt &
./working_set 2 1000 100 workload_rapid.txt 1 > logs/im_2_wr.txt &
./working_set 5 1000 100 workload_rapid.txt 1 > logs/im_5_wr.txt &

wait

#python3 generate_graphs.py
#echo "Progress 10/10"
