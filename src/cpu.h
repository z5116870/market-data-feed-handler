#include <sched.h>
#include <stdio.h>
#include <iostream>

// Pins a thread to a specific CPU, allowing it build state on that CPUs caches (TLB, L1, L2 etc.) and reduce the need for any
// context switches (paired with the scheduling priority function below, this will monopolize the CPU).
inline void pinToCpu(int cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    if (sched_setaffinity(pthread_self(), sizeof(mask), &mask) != 0) {
        printf("Failed to pin thread: %ld to core %d\n", pthread_self(), cpu_id);
        perror("pinToCpu()");
    }
    std::cout << "Pinned to core " << cpu_id << std::endl;
}

// Raise the priority of the current thread to max, allowing it to preempt all other threads
// of type SCHED_OTHER (using vruntime through CFS) and all lower priority SCHED_FIFO tasks
inline void raisePriority() {
    sched_param sch{};
    sch.sched_priority = 99;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch) != 0) {
        perror("Failed to set max scheduling priority for");
    }
}