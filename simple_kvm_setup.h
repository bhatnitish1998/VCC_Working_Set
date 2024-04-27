#ifndef VCC_WORKING_SET_SIMPLE_KVM_SETUP_H
#define VCC_WORKING_SET_SIMPLE_KVM_SETUP_H

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

// Page fault
uint32_t page_fault_count = 0;

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

void vm_init(struct vm *vm, size_t mem_size) {
    // Task State Segment address
    u_int32_t tss_address = 0xfffbd000;

    // Open device fd
    vm->dev_fd = open("/dev/kvm", O_RDWR);
    if (vm->dev_fd < 0) {
        perror("open /dev/kvm");
        exit(1);
    }

    // Make sure KVM version is 12
    int kvm_version = ioctl(vm->dev_fd, KVM_GET_API_VERSION, 0);
    if (kvm_version < 0) {
        perror("KVM_GET_API_VERSION");
        exit(1);
    }

    if (kvm_version != KVM_API_VERSION) {
        fprintf(stderr, "Got KVM api version %d, expected %d\n", kvm_version, KVM_API_VERSION);
        exit(1);
    }

    // Create VM
    vm->vm_fd = ioctl(vm->dev_fd, KVM_CREATE_VM, 0);
    if (vm->vm_fd < 0) {
        perror("KVM_CREATE_VM");
        exit(1);
    }

    // Set task state segment address
    if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, tss_address) < 0) {
        perror("KVM_SET_TSS_ADDR");
        exit(1);
    }

    // Allocate memory
    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem == MAP_FAILED) {
        perror("mmap mem");
        exit(1);
    }

    //Configure memory
    struct kvm_userspace_memory_region memreg;
    memreg.slot = 0;
    memreg.flags = 0;
    memreg.guest_phys_addr = 0;
    memreg.memory_size = mem_size;
    memreg.userspace_addr = (unsigned long) vm->mem;
    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        exit(1);
    }
}

void vcpu_init(struct vm *vm, struct vcpu *vcpu) {
    // Create vcpu
    vcpu->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpu->vcpu_fd < 0) {
        perror("KVM_CREATE_VCPU");
        exit(1);
    }

    // Get kvm_run memory size
    int vcpu_mmap_size = ioctl(vm->dev_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        exit(1);
    }

    // allocate memory for kvm_run
    vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu->vcpu_fd, 0);
    if (vcpu->kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        exit(1);
    }
}

static void setup_protected_mode(struct kvm_sregs *sregs) {
    struct kvm_segment seg = {.base = 0, .limit = 0xffffffff, .selector = 1
            << 3, // 2 privilege level bits and 1 table indicator bit, remaining index.
            .present = 1, .type = 11, // Code: execute, read, accessed
            .dpl = 0, .db = 1, .s = 1, // Code/data
            .l = 0, .g = 1, // 4KB granularity
    };

    sregs->cr0 |= CR0_PE; // enter protected mode

    sregs->cs = seg;

    seg.type = 3; // Data: read/write, accessed //
    seg.selector = 2 << 3;
    sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs) {
    uint32_t *pd = (void *) (vm->mem + pd_addr);
    uint32_t *pt = (void *) (vm->mem + pt_addr);

    // Set up page tables
    for (int i = 0; i < num_pages; ++i) {
        pt[i] = (i * page_size) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
    }

    // Set up page directory (2nd level) entries for page tables
    size_t num_page_tables = (num_pages * 4) / page_size; // each entry is 4 byte and covers 4MB memory.
    for (size_t i = 0; i < num_page_tables; ++i) {
        pd[i] = (pt_addr + i * page_size) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
    }

    // set page table related registers
    sregs->cr3 = pd_addr;
    sregs->cr4 = 0; // not using large pages
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->efer = 0;
}

void handle_pgfault(struct vm *vm, struct vcpu *vcpu) {
    struct kvm_sregs sregs;
    if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }
    uint32_t address = sregs.cr2;

    uint32_t *pt = (void *) (vm->mem + pt_addr);
    uint32_t page_num = address / 4096;
    pt[page_num] = pt[page_num] | PTE32_PRESENT;
    page_fault_count++;
}

void kvm_run_once(struct vm *vm, struct vcpu *vcpu) {
    int sc = ioctl(vcpu->vcpu_fd, KVM_RUN, 0);

    switch (vcpu->kvm_run->exit_reason) {
        case KVM_EXIT_HLT:
            printf("VM Halted\n");
            exit(1);

        case KVM_EXIT_IO:
            printf("KVM IO EXIT \n");
            break;

        case KVM_EXIT_SHUTDOWN:
            handle_pgfault(vm, vcpu);
            break;

        case KVM_EXIT_INTR:
            break;

        default:
            fprintf(stderr, "Got exit_reason %d, expected KVM_EXIT_HLT (%d)\n", vcpu->kvm_run->exit_reason,
                    KVM_EXIT_HLT);
            exit(1);
    }

    if (sc < 0 && vcpu->kvm_run->exit_reason != KVM_EXIT_INTR) {
        fprintf(stderr, "Failed running vm\n");
        exit(1);
    }
}

void load_binary(struct vm *vm, char *binary_file) {
    int fd = open(binary_file, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "can not open binary file\n");
        exit(1);
    }

    size_t ret;
    size_t total = 0;
    char *p = (char *) vm->mem;

    while (1) {
        ret = read(fd, p, 4096);
        if (ret <= 0)
            break;
        p += ret;
        total += ret;
    }
}

#endif //VCC_WORKING_SET_SIMPLE_KVM_SETUP_H