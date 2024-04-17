#include <stdint.h>

void hc_print_32_bit(uint32_t val)
{
    asm("outl %0,%1" : /* empty */ : "a" (val), "Nd" (0xE1) : "memory");
}

uint32_t hc_read_32_bit()
{
    uint32_t value;
    asm("inl %1,%0" :"=a" (value) : "Nd" (0xE2) : "memory");
    return value;
}

void hc_print_string(char *str)
{
    uint32_t addr =(uint32_t) ((uintptr_t) str);
    asm("outl %0,%1" : /* empty */ : "a" (addr), "Nd" (0xE3) : "memory");
}


int main ()
{
    for(int i =0; i<5; i++) {
        uint32_t exits = hc_read_32_bit();
        hc_print_32_bit(exits);
        char *str = "Read and printed number of exits\n";
        hc_print_string(str);
    }
    asm("hlt" : : :);

    return 0;
}
