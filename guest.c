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
    uint32_t exits = hc_read_32_bit();
    hc_print_32_bit(exits);

    asm("hlt" : : :);

    return 0;
}