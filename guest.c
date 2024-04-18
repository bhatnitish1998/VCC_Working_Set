#include <stdint.h>


void hc_print_32_bit(uint32_t val) {
    asm("outl %0,%1" : /* empty */ : "a" (val), "Nd" (0xE1) : "memory");
}

uint32_t hc_read_32_bit() {
    uint32_t value;
    asm("inl %1,%0" :"=a" (value) : "Nd" (0xE2) : "memory");
    return value;
}

int main() {

    long begin = 0x3200000; // 50 MB
    long end = 0x6400000; // 100 MB

    while(1)
    {
        for(long i=begin;i<=end;i+=0x1000)
        {
            *(long *) (+(i)) = 42;
        }
    }
    return 0;
}