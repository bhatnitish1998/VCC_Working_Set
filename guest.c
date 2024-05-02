#include <stdint.h>

// Random number generator for random memory access
// LCG Parameters
const long long a = 1103515245;
const long long c = 12345;
const long long m = 2147483648;
long long seed =123456789;

long lcg_rand() {
    long  rand = (a * seed + c) %m;
    seed = rand;
    return rand;
}

// guest parameter memory location
uint32_t mem_access_size_mb_addr = 0xA00000; // Memory usage parameter off guest (at 10 MB)
uint32_t mem_rand_perc_addr = 0xA00008; // Percentage of memory access that should be random
uint32_t overflow_counter_addr = 0xA00010; // To track performance of guest

// memory locations
long dummy_location_1 = 0xB00000; // Write to prevent compiler optimization (at 11 MB)
long dummy_location_2 = 0xB00010;
long begin = 0x1900000 ; // Start address for contiguous memory test.

// helper functions
long read_from_memory(uint32_t addr) {
    volatile long value = *(long *) addr;
    return value;
}

void write_to_memory(long value, uint32_t addr) {
    *(long *) addr = value;
}

// Increment the counter variable and increase overflow on reaching a million
int increment_counter() {
    volatile long value = read_from_memory(dummy_location_2);
    value++;
    write_to_memory(value,dummy_location_2);

    if (value == 1000000) {
        volatile long temp = read_from_memory(overflow_counter_addr);
        write_to_memory(temp+1,overflow_counter_addr);
        write_to_memory(0,dummy_location_2);
    }
    return value;
}

int main() {

    write_to_memory(0,overflow_counter_addr);
    write_to_memory(0,dummy_location_2);

    // infinitely access memory based on parameters.
    while(1)
    {
        // get access pattern from fixed locations
        volatile long mem_size_mb = read_from_memory(mem_access_size_mb_addr);
        volatile long mem_pages = mem_size_mb * 1024 / 4;
        volatile long rand_percent = read_from_memory(mem_rand_perc_addr);

        // determine number of random and contiguous pages to access
        long random_pages, contiguous_pages;
        if (rand_percent != 0) {
            random_pages = (mem_pages * rand_percent) / 100;
            contiguous_pages = mem_pages - random_pages;
        } else {
            random_pages = 0;
            contiguous_pages = mem_pages;
        }

        // Memory Workload
        long x;
        long sum =0;
        seed =123456789;

        // access random pages and increment counter.
        for (int i = 0; i < random_pages; i++) {
            long addr = lcg_rand() % 0x20000;
            x = read_from_memory(addr * 0x1000);
            sum += x;
            increment_counter();
        }

        // access contiguous pages and increment counter
        for (int j = 0; j < contiguous_pages; j++) {
            x = read_from_memory(begin + (j * 0x1000));
            sum += x;
            increment_counter();
        }

        // write to dummy location to avoid compiler optimization
        write_to_memory(sum,dummy_location_1);
    }

    return 0;
}