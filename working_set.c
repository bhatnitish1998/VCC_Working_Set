#include "simple_kvm_setup.h"
#include "memory_helper.h"
#include "timer_and_signals_helper.h"
#include "file_helper.h"

// Sampling parameters
int sample_interval;
int sample_size;

// experiment parameters
int random_percent;
int experiment_duration;
bool estimation_method;

// workload data structures
int workload_mem_size[30];
int workload_mem_duration[30];
int workload_entries = 0;
int workload_current = 0;

// variables for graph
int global_time;

size_t get_wss_accessed_bit(struct vm *vm, uint32_t sample_sz) {
    // Calculate ratio of pages accessed from sample. sampling is done uniformly.
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

    // print header
    printf("Time\tWSS\tCounter\n");

    register_handler();

    // create and setup timers
    timer_t timer_sample, timer_pattern, timer_end;
    setup_timer(&timer_sample, SIGUSR1, sample_interval, 1);
    setup_timer(&timer_pattern, SIGUSR2, workload_mem_duration[workload_current], 1);
    setup_timer(&timer_end, SIGALRM, experiment_duration, 1);

    // set initial guest parameters
    write_to_vm_memory(vm, workload_mem_size[workload_current], mem_access_size_mb_addr);
    workload_current++;
    write_to_vm_memory(vm, random_percent, mem_rand_perc_addr);

    while (1) {
        kvm_run_once(vm, vcpu);

        // Calculate Working set size
        if (sample_signal == 1) {
            // Access bit method or invalidation method.
            size_t wss;
            if(estimation_method == 0)
                wss = get_wss_accessed_bit(vm, sample_size);
            else
                wss = get_wss_invalidation(vm, sample_size);


            // print details
            global_time+=sample_interval;
            long counter_value = read_from_vm_memory(vm,overflow_counter_addr);
            printf("%d\t",global_time);
            print_memory_size(wss);
            printf("\t%ld\n",counter_value);


            sample_signal = 0;

        }

        // Change guest access pattern
        if (change_mem_access_pattern == 1) {
            if (workload_current <= workload_entries) {
                write_to_vm_memory(vm, workload_mem_size[workload_current], mem_access_size_mb_addr);
                setup_timer(&timer_pattern, SIGUSR2, workload_mem_duration[workload_current], 0);
                workload_current++;
            }
            change_mem_access_pattern = 0;
        }

        if (end_signal == 1) {
            break;
        }
    }
}

void run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu) {

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

int main(int argc, char *argv[]) {

    if (argc != 6) {
        printf("Usage: %s <sample_interval> <sample_size> <random_percent> <workload file> <estimation method>\n", argv[0]);
        return 1;
    }

    sample_interval = atoi(argv[1]);
    sample_size = atoi(argv[2]);
    random_percent = atoi(argv[3]);
    const char *workload_file = argv[4];
    estimation_method = atoi(argv[5]);


    // Initialize VM and VCPU
    struct vm vm;
    struct vcpu vcpu;
    vm_init(&vm, memory_size);
    vcpu_init(&vm, &vcpu);

    workload_entries = read_workload_from_file(workload_file, workload_mem_size, workload_mem_duration);
    for(int i =0 ;i < workload_entries; i++)
        experiment_duration += workload_mem_duration[i];

    // Run VM in 32 bit paged mode.
    run_paged_32bit_mode(&vm, &vcpu);

    return 0;
}