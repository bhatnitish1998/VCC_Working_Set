#ifndef VCC_WORKING_SET_TIMER_AND_SIGNALS_HELPER_H
#define VCC_WORKING_SET_TIMER_AND_SIGNALS_HELPER_H

#include <signal.h>
#include <time.h>
#include <stdbool.h>

// signal variables
bool sample_signal = 0;
bool change_mem_access_pattern = 0;
bool end_signal = 0;

void setup_timer(timer_t *timer_id, int signal, int interval, int new) {
    if (new == 1) {
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

static void handler(int sig) {
    if (sig == SIGUSR1)
        sample_signal = 1;
    if (sig == SIGUSR2)
        change_mem_access_pattern = 1;
    if (sig == SIGALRM)
        end_signal = 1;
}

void register_handler() {
    struct sigaction sa;
    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *)) handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1 ||
        sigaction(SIGALRM, &sa, NULL) == -1)
        perror("sigaction");
}

#endif //VCC_WORKING_SET_TIMER_AND_SIGNALS_HELPER_H
