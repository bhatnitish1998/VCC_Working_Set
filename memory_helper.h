#ifndef VCC_WORKING_SET_MEMORY_HELPER_H
#define VCC_WORKING_SET_MEMORY_HELPER_H

#include "simple_kvm_setup.h"

// guest parameter memory location
uint32_t mem_access_size_mb_addr = 0xA00000; // Memory usage parameter off guest (at 10 MB)
uint32_t mem_rand_perc_addr = 0xA00008; // Percentage of memory access that should be random
uint32_t overflow_counter_addr = 0xA00010; // To track performance of guest

void print_memory_size(size_t bytes) {
    if ((bytes / 1024) <= 1) {
        printf("%lu bytes", bytes);
        return;
    }
    bytes = bytes / 1024;
    if ((bytes / 1024) <= 1) {
        printf("%lu KB", bytes);
        return;
    }
    bytes = bytes / 1024;
    if ((bytes / 1024) <= 1) {
        printf("%lu MB", bytes);
        return;
    }
}

void write_to_vm_memory(struct vm *vm, long value, uint32_t address) {
    long *ptr = (long *) ((char *) vm->mem + address);
    *ptr = value;
}

long read_from_vm_memory(struct vm *vm, uint32_t address) {
    long value = *(long *) (vm->mem + address);
    return value;
}

#endif //VCC_WORKING_SET_MEMORY_HELPER_H
