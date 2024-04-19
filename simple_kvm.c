#include "simple_kvm.h"

// VM configuration
const size_t memory_size = 0x20000000; // 512 MB

// Page table configuration
const uint32_t pd_addr = 0x500000; // Page directory address (at 5 MB)
const uint32_t pt_addr = 0x600000; // Start of page tables  (at 6 MB)
const uint32_t page_size = 0x1000;  // 4KB
const uint32_t stack_pointer = 0x20000000; // at end
const uint32_t num_pages = memory_size / page_size;

// guest parameter memory location
uint32_t mem_access_size_addr = 0xA00000; // Memory usage parameter off guest (at 10 MB)
uint32_t mem_rand_perc_addr = 0xA00008; // Percentage of memory access that should be random
uint32_t overflow_counter_addr = 0xA00010; // To track performance of guest

// Hyper call info (if needed)
const uint32_t hc_print_32_bit = 0xE1;
const uint32_t hc_read_32_bit = 0xE2;

// Guest binary
char *guest_binary = "guest.bin";

// Sampling parameters
int sample_interval = 5;
int mem_pattern_change_interval = 12;
uint32_t sample_size = 1000;

// signal variables
int sample_signal = 0;
int pattern_signal = 0;
int end_signal = 0;

// workload
uint32_t mem_pattern [] ={50, 150 , 100 , 200, 50};
int time_span []={12,12,12,12,12};
int num_pattern = 5;
int pattern_index =0;


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

size_t get_wss_accessed_bit(struct vm *vm, uint32_t sample_sz) {
    // Calculate ratio of pages access from sample. sampling is done uniformly.
    uint32_t distance = num_pages / sample_sz;
    uint32_t *pt = (void *) (vm->mem + pt_addr);
    int count = 0;
    for (uint32_t i = 0; i < num_pages; i += distance) {
        if (pt[i] & PTE32_ACCESSED) {
            pt[i] = pt[i] & ~(PTE32_ACCESSED);
            count++;
        }
    }

    // estimate working set size;
    double ratio = (double) count / sample_sz;
    size_t memory_estimate = (size_t) (ratio * num_pages) * page_size;
    return memory_estimate;
}

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
    sregs->cr0
            = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->efer = 0;
}

void kvm_run_once(struct vcpu *vcpu) {
    int sc = ioctl(vcpu->vcpu_fd, KVM_RUN, 0);

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
                int write_value = 50;
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
            fprintf(stderr, "Got exit_reason %d, expected KVM_EXIT_HLT (%d)\n",
                    vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
            exit(1);
    }

    if (sc < 0 && vcpu->kvm_run->exit_reason != KVM_EXIT_INTR) {
        fprintf(stderr, "Failed running vm\n");
        exit(1);
    }
}

static void handler(int sig) {
    if (sig == SIGUSR1)
        sample_signal = 1;
    if (sig == SIGUSR2)
        pattern_signal = 1;
    if (sig == SIGALRM)
        end_signal =1;
}

void setup_timer (timer_t * timer_id, int signal, int interval,int new)
{
    if(new ==1)
    {
        struct sigevent sev;
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = signal;
        sev.sigev_value.sival_ptr = timer_id;
        if (timer_create(CLOCK_REALTIME, &sev, timer_id) == -1)
            perror("timer_create");
    }

    struct itimerspec its;
    its.it_value.tv_sec = interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = interval;
    its.it_interval.tv_nsec = 0;
    if (timer_settime(*timer_id, 0, &its, NULL) == -1)
        perror("timer_settime");
}

void run_vm(struct vm *vm, struct vcpu *vcpu) {
    // establish handler
    struct sigaction sa;
    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *)) handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1 || sigaction(SIGALRM, &sa, NULL) == -1)
        perror("sigaction");

    // create and setup timers
    timer_t timer_sample , timer_pattern,timer_end;
    setup_timer(&timer_sample,SIGUSR1,5,1);
    setup_timer(&timer_pattern,SIGUSR2,time_span[pattern_index],1);
    setup_timer(&timer_end,SIGALRM,60,1);

    write_to_memory(vm,mem_pattern[pattern_index],mem_access_size_addr);
    pattern_index++;

    write_to_memory(vm, 100, mem_rand_perc_addr);

    while (1) {
        kvm_run_once(vcpu);

        // Calculate Working set size
        if (sample_signal == 1) {
            size_t wss = get_wss_accessed_bit(vm, sample_size);
            printf("Working Set Size :");
            print_memory(wss);
            printf("\n");
            sample_signal = 0;
        }

        // Change guest access pattern
        if (pattern_signal == 1) {
            if(pattern_index <= num_pattern)
            {
                write_to_memory(vm,mem_pattern[pattern_index],mem_access_size_addr);
                pattern_index++;
            }
            pattern_signal = 0;
        }

        if(end_signal ==1)
        {
            long counter_value = *(long *)(vm->mem + overflow_counter_addr);
            printf("Final counter overflows %ld",counter_value);
            break;
        }
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
    run_vm(vm, vcpu);
}


int main() {

    // Initialize VM and VCPU
    struct vm vm;
    struct vcpu vcpu;
    vm_init(&vm, memory_size);
    vcpu_init(&vm, &vcpu);

    // Run VM in 32 bit paged mode.
    run_paged_32bit_mode(&vm, &vcpu);
    long counter_value = *(long *)(vm.mem + overflow_counter_addr);
    printf("Final counter overflows %ld",counter_value);

    struct workload1;

    return 0;
}