#include <stdint.h>

long result_location = 0x1600000; // 25 MB
long begin = 50 *1024*1024 ; // 50 MB
uint32_t mem_access_size_addr = 0xA00000;
uint32_t mem_random_addr = 0xA00008; // Percentage of memory access that should be random
uint32_t overflow_counter_addr = 0xA00010; // To track performance of guest
uint32_t randomness_addr = 0xA00018; // 0 for contiguous 1 for random access.

const long long a = 1103515245;
const long long c = 12345;
const long long m = 2147483648;
long long seed =123456789;

long lcg_rand() {
    long  rand = (a * seed + c) %m;
    seed = rand;
    return rand;
}

int increment_counter(int value)
{
    value++;
    if(value ==10000000)
    {
        long t = *(long*) overflow_counter_addr;
        *(long*) overflow_counter_addr =t+1;
        value =0;
    }
    return value;
}

void hc_print_32_bit(uint32_t val) {
    asm("outl %0,%1" : /* empty */ : "a" (val), "Nd" (0xE1) : "memory");
}

uint32_t hc_read_32_bit() {
    uint32_t value;
    asm("inl %1,%0" :"=a" (value) : "Nd" (0xE2) : "memory");
    return value;
}

int main() {

    *(long*) overflow_counter_addr =0;
    int k =0;

    while(1)
    {
        volatile long mem_pages = (*((long*) mem_access_size_addr))*1024 /4;
        volatile long rand_percent = *(long*) mem_random_addr;

        long random_pages;
        long contiguous_pages;
        if(rand_percent !=0)
        {
            random_pages = (mem_pages * rand_percent)/100;
            contiguous_pages = mem_pages - random_pages;
        }
        else
        {
            random_pages =0;
            contiguous_pages =mem_pages;
        }

        long x;
        long sum =0;

        seed =123456789;
        for( int i =0; i< random_pages; i++)
        {
            long addr = lcg_rand() % 0x20000;
            x = *(long *)(addr*0x1000);
            sum += x;
            k= increment_counter(k);
        }

        for( int j =0; j< contiguous_pages; j++) {
            x = *(long *) (begin + (j * 0x1000));
            sum+=x;
            k=increment_counter(k);
        }

        *(long *) result_location = sum;
    }
    return 0;
}