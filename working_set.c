#include "simple_kvm_setup.h"
#include "memory_helper.h"
#include "timer_and_signals_helper.h"


// Sampling parameters
int sample_interval = 5;
int sample_size = 1000;
int random_percent;
bool method;

// workload
int mem_pattern[30];
int time_span[30];
int num_pattern = 0;
int pattern_index = 0;

int read_workload(const char *filename, int access_size[], int duration[]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return 0;
    }

    int rows =0;
    // Skip the first line (header)
    char line[100];
    fgets(line, sizeof(line), file);

    // Read the rest of the file
    while (fscanf(file, "%d %d", &access_size[rows], &duration[rows]) == 2) {
        rows++;
    }
    fclose(file);

    return rows;
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

size_t get_wss_invalidation(struct vm *vm, uint32_t sample_sz) {
    double ratio = (double) page_fault_count / sample_sz;
    size_t memory_estimate = (size_t) (ratio * num_pages) * page_size;
    page_fault_count = 0;

    // Invalidate sample pages
    uint32_t distance = num_pages / sample_sz;
    uint32_t *pt = (void *) (vm->mem + pt_addr);
    for (uint32_t i = 0; i < num_pages; i += distance) {
        pt[i] = pt[i] & ~(PTE32_PRESENT);
    }
    return memory_estimate;
}

void run_vm(struct vm *vm, struct vcpu *vcpu) {

    register_handler();
    // create and setup timers
    timer_t timer_sample, timer_pattern, timer_end;
    setup_timer(&timer_sample, SIGUSR1, 5, 1);
    setup_timer(&timer_pattern, SIGUSR2, time_span[pattern_index], 1);
    setup_timer(&timer_end, SIGALRM, 60, 1);

    write_to_vm_memory(vm, mem_pattern[pattern_index], mem_access_size_mb_addr);
    pattern_index++;

    write_to_vm_memory(vm, random_percent, mem_rand_perc_addr);

    while (1) {
        kvm_run_once(vm, vcpu);

        // Calculate Working set size
        if (sample_signal == 1) {
            //size_t wss = get_wss_accessed_bit(vm, sample_size);
            size_t wss = get_wss_invalidation(vm, sample_size);
            printf("Working Set Size :");
            print_memory_size(wss);
            printf("\n");
            sample_signal = 0;
        }

        // Change guest access pattern
        if (change_mem_access_pattern == 1) {
            if (pattern_index <= num_pattern) {
                write_to_vm_memory(vm, mem_pattern[pattern_index], mem_access_size_mb_addr);
                setup_timer(&timer_pattern, SIGUSR2, time_span[pattern_index], 0);
                pattern_index++;
            }
            change_mem_access_pattern = 0;
        }

        if (end_signal == 1) {
            long counter_value = read_from_vm_memory(vm, overflow_counter_addr);
            printf("Final counter overflows %ld", counter_value);
            break;
        }
    }
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

    num_pattern = read_workload("workload1.txt",mem_pattern,time_span);

    // Run VM in 32 bit paged mode.
    run_paged_32bit_mode(&vm, &vcpu);

    // check final counter value for performance.
    long counter_value = *(long *) (vm.mem + overflow_counter_addr);
    printf("Final counter overflows %ld", counter_value);

    return 0;
}