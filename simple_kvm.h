#ifndef VCC_WORKING_SET_SIMPLE_KVM_H
#define VCC_WORKING_SET_SIMPLE_KVM_H

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

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)


/* 32-bit page directory entry bits */
#define PTE32_PRESENT 1
#define PTE32_RW (1U << 1)
#define PTE32_USER (1U << 2)
#define PTE32_ACCESSED (1U << 5)
#define PTE32_DIRTY (1U << 6)
#define PTE32_PS (1U << 7)
#define PTE32_G (1U << 8)

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


// function definitions
void vm_init(struct vm *vm, size_t mem_size);
void vcpu_init(struct vm *vm, struct vcpu *vcpu);
static void setup_protected_mode(struct kvm_sregs *sregs);
static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs);
void kvm_run_once(struct vcpu * vcpu);

_Noreturn void run_vm(struct vcpu *vcpu);
void load_binary(struct vm *vm, char *binary_file);
void run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu);



#endif //VCC_WORKING_SET_SIMPLE_KVM_H
