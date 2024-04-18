#include "simple_kvm.h"

// VM configuration
const size_t memory_size = 0x20000000; // 512 MB

// Page table configuration
const uint32_t pd_addr = 0xA00000; // Page directory address (at 10 MB)
const uint32_t pt_addr = 0xA10000; // Start of page tables (4KB from directory)
const uint32_t page_size = 0x1000;  // 4KB
const uint32_t stack_pointer = 128 << 20; // at 128 MB

// Hyper call port numbers
const uint32_t hc_print_32_bit = 0xE1;
const uint32_t hc_read_32_bit = 0xE2;

// Write value to VM
int write_value = 50;

// interrupt parameters
const int time_interval = 1;

// Guest binary
char *guest_binary = "guest.bin";


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
        fprintf(stderr, "Got KVM api version %d, expected %d\n",
                kvm_version, KVM_API_VERSION);
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
    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
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
    vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                         MAP_SHARED, vcpu->vcpu_fd, 0);
    if (vcpu->kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        exit(1);
    }
}

static void setup_protected_mode(struct kvm_sregs *sregs) {
    struct kvm_segment seg = {
            .base = 0,
            .limit = 0xffffffff,
            .selector = 1 << 3, // 2 privilege level bits and 1 table indicator bit, remaining index.
            .present = 1,
            .type = 11, // Code: execute, read, accessed
            .dpl = 0,
            .db = 1,
            .s = 1, // Code/data
            .l = 0,
            .g = 1, // 4KB granularity
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
    for (int i = 0; i < (memory_size / page_size); ++i) {
        pt[i] = (i * page_size) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
    }

    // Set up page directory entries for page tables
    size_t num_page_tables = memory_size / (page_size * 1024 * 4); // Each page table covers 4MB
    for (size_t i = 0; i < num_page_tables; ++i) {
        pd[i] = (pt_addr + i * 0x1000) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
    }

    // set page table related registers
    sregs->cr3 = pd_addr;
    sregs->cr4 = 0; // not using large pages
    sregs->cr0
            = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->efer = 0;
}

void kvm_run_once(struct vcpu *vcpu) {

    int sc = ioctl(vcpu->vcpu_fd,KVM_RUN,0);

    switch (vcpu->kvm_run->exit_reason) {
        case KVM_EXIT_HLT:
            printf("VM Halted\n");
            exit(1);
        case KVM_EXIT_IO:
            // Print 32-bit integer
            if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT
                && vcpu->kvm_run->io.port == hc_print_32_bit) {
                // get the location where 32-bit number is present from offset.
                char *res = (char *) vcpu->kvm_run + vcpu->kvm_run->io.data_offset;
                u_int32_t *output = (u_int32_t *) res;
                printf("Print number : %u\n", *output);
                break;

                // Read 32-bit integer
            } else if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_IN
                       && vcpu->kvm_run->io.port == hc_read_32_bit) {
                // write value to the location given by the offset.
                char *res = (char *) vcpu->kvm_run + vcpu->kvm_run->io.data_offset;
                u_int32_t *input = (u_int32_t *) res;
                *input = write_value;
                printf("Read number : %u\n", write_value);
                break;
            }

        case KVM_EXIT_SHUTDOWN:
            fprintf(stderr, "KVM SHUTDOWN\n");
            exit(1);

        case KVM_EXIT_INTR:
            printf("KVM Interrupted \n");
            break;

        default:
            fprintf(stderr, "Got exit_reason %d,"
                            " expected KVM_EXIT_HLT (%d)\n",
                    vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
            exit(1);
    }

    if (sc < 0 && vcpu->kvm_run->exit_reason != KVM_EXIT_INTR)
    {
        fprintf(stderr, "Failed running vm\n");
        exit(1);
    }
}

_Noreturn void run_vm(struct vcpu *vcpu) {
    // create timer
    struct sigevent sev;
    timer_t timerid;
    struct itimerspec  its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
        perror("timer create");

    // block the signal
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        perror("sigprocmask");

    // ioctl call to unmask in VMX mode
    struct kvm_signal_mask kmask;
    kmask.len=sizeof(unsigned long);
    sigemptyset((sigset_t *)kmask.sigset);

    ioctl(vcpu->vcpu_fd,KVM_SET_SIGNAL_MASK,&kmask);

    // start timer with interval of 1 sec
    its.it_value.tv_sec = time_interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = time_interval;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(timerid, 0, &its, NULL) == -1)
        perror("timer_settime");


    while (1) {
        kvm_run_once(vcpu);

        // handle the signal
        sigset_t to_check;
        int sig;
        sigaddset(&to_check,SIGALRM);
        sigwait(&to_check,&sig);
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
    printf("Loaded Program with size: %lu\n", total);
}

void run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu) {
    printf("Running in 32-bit paged mode\n");

    // set special register values
    struct kvm_sregs sregs;

    if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_protected_mode(&sregs);
    setup_paged_32bit_mode(vm, &sregs);

    if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    // set general register values
    struct kvm_regs regs;

    memset(&regs, 0, sizeof(regs));
    regs.rflags = 2; // last but 1 bit is always set.
    regs.rip = 0;
    regs.rsp = stack_pointer;

    if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

    load_binary(vm, guest_binary);
    run_vm(vcpu);
}


int main() {
    // Initialize VM and VCPU
    struct vm vm;
    struct vcpu vcpu;
    vm_init(&vm, memory_size);
    vcpu_init(&vm, &vcpu);

    // Run VM in 32 bit paged mode.
    run_paged_32bit_mode(&vm, &vcpu);

    return 0;
}