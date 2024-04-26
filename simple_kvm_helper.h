#ifndef VCC_WORKING_SET_SIMPLE_KVM_HELPER_H
#define VCC_WORKING_SET_SIMPLE_KVM_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <signal.h>
#include <time.h>
#include <string.h>

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* 32-bit page directory entry bits */
#define PTE32_PRESENT 1
#define PTE32_RW (1U << 1)
#define PTE32_USER (1U << 2)
#define PTE32_ACCESSED (1U << 5)
#define PTE32_DIRTY (1U << 6)
#define PTE32_PS (1U << 7)
#define PTE32_G (1U << 8)

// VM Configuration

//Memory size
const size_t memory_size = 0x20000000; // 512 MB

// Page table configuration
const uint32_t pd_addr = 0x500000; // Page directory address (at 5 MB)
const uint32_t pt_addr = 0x600000; // Start of page tables  (at 6 MB)
const uint32_t page_size = 0x1000;  // 4KB
const uint32_t stack_pointer = 0x20000000; // at end
const uint32_t num_pages = memory_size / page_size;

// guest parameter memory location
uint32_t mem_access_size_mb_addr = 0xA00000; // Memory usage parameter off guest (at 10 MB)
uint32_t mem_rand_perc_addr = 0xA00008; // Percentage of memory access that should be random
uint32_t overflow_counter_addr = 0xA00010; // To track performance of guest

// Guest binary
char *guest_binary = "guest.bin";

// structure definitions
struct vm {
    int dev_fd;
    int vm_fd;
    char *mem;
};

struct vcpu {
    int vcpu_fd;
    struct kvm_run *kvm_run;
};

struct workload
{
    int size [30];
    int time [30];
    int num_elements;
    int current;
};

// helper functions

void print_memory(size_t bytes) {
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

void write_to_memory (struct vm *vm, long value , uint32_t address)
{
    long *ptr = (long *)((char*)vm->mem + address);
    *ptr = value;
}

long read_from_memory (struct vm *vm, uint32_t address)
{
    long  value = *(long *)(vm->mem + address);
    return value;
}


#endif //VCC_WORKING_SET_SIMPLE_KVM_HELPER_H
