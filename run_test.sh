#!/bin/bash

# ./simple_kvm <sample_interval> <sample_size> <random_percent> <workload file> <estimation method>

# access bit method vs page invalidation method performance
# performance for different sample size
./simple_kvm 3 100 100 workload1.txt 0 > am_100.txt
./simple_kvm 3 1000 100 workload1.txt 0 > am_1000.txt
./simple_kvm 3 10000 100 workload1.txt 0 > am_10000.txt

./simple_kvm 3 100 100 workload1.txt 1 > im_100.txt
./simple_kvm 3 1000 100 workload1.txt 1 > im_1000.txt
./simple_kvm 3 10000 100 workload1.txt 1 > im_10000.txt

# sampling size accuracy when memory access pattern are randomized.
./simple_kvm 3 100 0 workload1.txt 1 > r_0_100.txt
./simple_kvm 3 1000 0 workload1.txt 1 > r_0_1000.txt
./simple_kvm 3 10000 0 workload1.txt 1 > r_0_10000.txt

./simple_kvm 3 100 20 workload1.txt 1 > r_20_100.txt
./simple_kvm 3 1000 20 workload1.txt 1 > r_20_1000.txt
./simple_kvm 3 10000 20 workload1.txt 1 > r_20_10000.txt

./simple_kvm 3 100 100 workload1.txt 1 > r_100_100.txt
./simple_kvm 3 1000 100 workload1.txt 1 > r_100_1000.txt
./simple_kvm 3 10000 100 workload1.txt 1 > r_100_10000.txt

# sampling rate for different workload.
./simple_kvm 3 1000 100 workload1.txt 1 > im_1_w1.txt
./simple_kvm 3 1000 100 workload2.txt 1 > im_1_w2.txt

./simple_kvm 3 1000 100 workload1.txt 1 > im_3_w1.txt
./simple_kvm 3 1000 100 workload2.txt 1 > im_3_w2.txt

./simple_kvm 5 1000 100 workload1.txt 1 > im_5_w1.txt
./simple_kvm 5 1000 100 workload2.txt 1 > im_5_w2.txt

./simple_kvm 10 1000 100 workload1.txt 1 > im_10_w1.txt
./simple_kvm 10 1000 100 workload2.txt 1 > im_10_w2.txt

