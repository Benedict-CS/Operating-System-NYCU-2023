Assignment 2: Implement a program to apply different scheduling policies on created threads
===
- Assignment 2: Implement a program to apply different scheduling policies on created threads
  - Linux Scheduling Policy
    - `SCHED_FIFO`
  - Requirements
    - Main thread
    - Worker Thread

Implement a program to apply different scheduling policies on created threads
---
### Q1: Describe how you implemented the program in detail.

**Step 0: Create new program**
```
vim sched_demo_312551002.c
```
**Step 1: Define and include related library**
```c
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
```

I enabled the GNU extensions because I'll using functions like `CPU_ZERO`, `CPU_SET`, and `pthread_setaffinity_np`, if not enabled the GNU extensions, when compile will occur errors and warnings. 

**Step 2: Define a struct for collecting required thread information**
```c
typedef struct {
    pthread_t thread_id;
    int thread_num;
    int sched_policy;
    int sched_priority;
    pthread_barrier_t *barrier;
    double time_wait;
} thread_info_t;
```

**Step 3: Implement the Parse program arguments**
```
testcases:

-n 1 -t 0.5 -s NORMAL -p -1
-n 2 -t 0.5 -s FIFO,FIFO -p 10,20
-n 3 -t 1.0 -s NORMAL,FIFO,FIFO -p -1,10,30
```

```c
void parse_args(int argc, char *argv[], int *num_threads, double *time_wait, 
                char ***policies_array, char ***priorities_array) 
{
    char *policies_str = NULL;
    char *priorities_str = NULL;
    
    /* Parse program arguments */
    int opt;
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':
                *num_threads = atoi(optarg);
                break;
            case 't':
                *time_wait = atof(optarg);
                break;
            case 's':
                policies_str = optarg;
                break;
            case 'p':
                priorities_str = optarg;
                break;
        }
    }

    /* Split policies and priotities argument */
    *policies_array = malloc(*num_threads * sizeof(char *));
    *priorities_array = malloc(*num_threads * sizeof(char *));

    char *rest_policy = policies_str;
    char *rest_priority = priorities_str;

    for (int i = 0; i < *num_threads; i++) {
        if (i == 0) {
            (*policies_array)[i] = strtok_r(policies_str, ",", &rest_policy);
            (*priorities_array)[i] = strtok_r(priorities_str, ",", &rest_priority);
        } 
        else {
            (*policies_array)[i] = strtok_r(NULL, ",", &rest_policy);
            (*priorities_array)[i] = strtok_r(NULL, ",", &rest_priority);
        }
    }
}
```

- Command-line arguments are parsed using the `getopt` function. The arguments include the number of threads `-n`, time to wait `-t`, scheduling policies `-s`, and priorities `-p`. The values are stored in appropriate variables for later use.
- Special attention is given to the `-s` and `-p` arguments, which can have multiple values separated by `","`. These values are split and stored in arrays for individual thread configurations.

**Step 4: Set CPU affinity and thread attribute**
```c
void set_thread_attributes(pthread_attr_t *attr, int sched_policy, int sched_priority) {
    
    /* Set CPU affinity */
    cpu_set_t cpuset;
    pthread_attr_init(attr);
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpuset);

    /* Set FIFO priority and attribute */
    if (sched_policy == SCHED_FIFO) {
        struct sched_param param;
        param.sched_priority = sched_priority;
        pthread_attr_setschedpolicy(attr, SCHED_FIFO);
        pthread_attr_setschedparam(attr, &param);
        pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    }
}
```

- `CPU_ZERO` and `CPU_SET` are used to bind threads to a specific CPU (CPU 0 in this case), ensuring that they run on the designated processor.
- If the scheduling policy is `SCHED_FIFO`, additional attributes such as scheduling policy and priority are set. `SCHED_FIFO` is a real-time scheduling policy where a thread runs until it either finishes or is preempted by a higher priority thread. If scheduling policy is `NORMAL` will skip this part.

**Step 5: Implement the Worker Thread**
```c
void *thread_func(void *arg) {
    thread_info_t *tinfo = (thread_info_t *)arg;
    struct timespec start, current;
    
    /* Wait until all threads are ready */
    pthread_barrier_wait(tinfo->barrier);

    /* Do the task */
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", tinfo->thread_num);
        clock_gettime(CLOCK_MONOTONIC, &start);
        double elapsed = 0;

        /* Busy for <time_wait> seconds */
        while (elapsed < tinfo->time_wait) {
            clock_gettime(CLOCK_MONOTONIC, &current);
            elapsed = current.tv_sec - start.tv_sec + 
                     (current.tv_nsec - start.tv_nsec) / 1000000000.0;
        }
    }

    /* Exit the function  */
    return NULL;
}
```

- The worker thread's function is defined here. Each thread waits at the barrier until all threads are ready to start simultaneously. This ensures synchronized start across all threads.
- Inside the loop, each thread performs its task for a specified duration `time_wait`. This is achieved using a busy-wait loop, calculating elapsed time with `clock_gettime`.

**Step 6: Implement the Main Thread**
```c
int main(int argc, char *argv[]) {
    
    /* Parse program arguments */
    int num_threads;
    double time_wait;
    char **policies;
    char **priorities;

    parse_args(argc, argv, &num_threads, &time_wait, &policies, &priorities);

    /* Initialize a barrier for synchronizing the start of the worker threads */
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_threads);

    /* Allocate memory for storing thread information */
    thread_info_t *tinfo = calloc(num_threads, sizeof(*tinfo));

    /* Create <num_threads> worker threads */
    for (int i = 0; i < num_threads; i++) {
        tinfo[i].thread_num = i;
        tinfo[i].time_wait = time_wait;
        tinfo[i].barrier = &barrier;
        
        /* Set thread scheduling policy and priority */
        if (strcmp(policies[i], "FIFO") == 0)
            tinfo[i].sched_policy = SCHED_FIFO;
        else
            tinfo[i].sched_policy = SCHED_OTHER;
        
        tinfo[i].sched_priority = atoi(priorities[i]);

        /* Set attributes to each thread, including CPU affinity and scheduling */
        pthread_attr_t attr;
        set_thread_attributes(&attr, tinfo[i].sched_policy, tinfo[i].sched_priority);
        
        /* Start the thread */
        pthread_create(&tinfo[i].thread_id, &attr, thread_func, &tinfo[i]);
        pthread_attr_destroy(&attr);
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(tinfo[i].thread_id, NULL);
    }

    /* Clean up */
    pthread_barrier_destroy(&barrier);
    free(tinfo);
    free(policies);
    free(priorities);

    return 0;
}
```

- The main function initializes the threading environment and starts the worker threads.
- It begins by parsing command-line arguments to determine the number of threads, their wait times, scheduling policies, and priorities.
- A barrier is initialized to synchronize the start of all worker threads.
- Threads are created in a loop. Each thread is given a unique number, its wait time, a reference to the barrier, and its scheduling policy and priority.
- Thread attributes are set according to the specified scheduling policy and CPU affinity.
- Threads are then started with `pthread_create`.
- Finally, the main thread waits for all worker threads to complete using `pthread_join` and performs necessary cleanup operations like destroying the barrier and freeing allocated memory.

**Complete sched_demo_312551002.c program like below:**
```c
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

typedef struct {
    pthread_t thread_id;
    int thread_num;
    int sched_policy;
    int sched_priority;
    pthread_barrier_t *barrier;
    double time_wait;
} thread_info_t;

void parse_args(int argc, char *argv[], int *num_threads, double *time_wait, 
                char ***policies_array, char ***priorities_array) 
{
    char *policies_str = NULL;
    char *priorities_str = NULL;
    
    /* Parse program arguments */
    int opt;
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':
                *num_threads = atoi(optarg);
                break;
            case 't':
                *time_wait = atof(optarg);
                break;
            case 's':
                policies_str = optarg;
                break;
            case 'p':
                priorities_str = optarg;
                break;
        }
    }

    /* Split policies and priotities argument */
    *policies_array = malloc(*num_threads * sizeof(char *));
    *priorities_array = malloc(*num_threads * sizeof(char *));

    char *rest_policy = policies_str;
    char *rest_priority = priorities_str;

    for (int i = 0; i < *num_threads; i++) {
        if (i == 0) {
            (*policies_array)[i] = strtok_r(policies_str, ",", &rest_policy);
            (*priorities_array)[i] = strtok_r(priorities_str, ",", &rest_priority);
        } 
        else {
            (*policies_array)[i] = strtok_r(NULL, ",", &rest_policy);
            (*priorities_array)[i] = strtok_r(NULL, ",", &rest_priority);
        }
    }
}

void set_thread_attributes(pthread_attr_t *attr, int sched_policy, int sched_priority) {
    
    /* Set CPU affinity */
    cpu_set_t cpuset;
    pthread_attr_init(attr);
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpuset);

    /* Set FIFO priority and attribute */
    if (sched_policy == SCHED_FIFO) {
        struct sched_param param;
        param.sched_priority = sched_priority;
        pthread_attr_setschedpolicy(attr, SCHED_FIFO);
        pthread_attr_setschedparam(attr, &param);
        pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    }
}

void *thread_func(void *arg) {
    thread_info_t *tinfo = (thread_info_t *)arg;
    struct timespec start, current;
    
    /* Wait until all threads are ready */
    pthread_barrier_wait(tinfo->barrier);

    /* Do the task */
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", tinfo->thread_num);
        clock_gettime(CLOCK_MONOTONIC, &start);
        double elapsed = 0;

        /* Busy for <time_wait> seconds */
        while (elapsed < tinfo->time_wait) {
            clock_gettime(CLOCK_MONOTONIC, &current);
            elapsed = current.tv_sec - start.tv_sec + 
                     (current.tv_nsec - start.tv_nsec) / 1000000000.0;
        }
    }

    /* Exit the function  */
    return NULL;
}

int main(int argc, char *argv[]) {
    
    /* Parse program arguments */
    int num_threads;
    double time_wait;
    char **policies;
    char **priorities;

    parse_args(argc, argv, &num_threads, &time_wait, &policies, &priorities);

    /* Initialize a barrier for synchronizing the start of the worker threads */
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_threads);

    /* Allocate memory for storing thread information */
    thread_info_t *tinfo = calloc(num_threads, sizeof(*tinfo));

    /* Create <num_threads> worker threads */
    for (int i = 0; i < num_threads; i++) {
        tinfo[i].thread_num = i;
        tinfo[i].time_wait = time_wait;
        tinfo[i].barrier = &barrier;
        
        /* Set thread scheduling policy and priority */
        if (strcmp(policies[i], "FIFO") == 0)
            tinfo[i].sched_policy = SCHED_FIFO;
        else
            tinfo[i].sched_policy = SCHED_OTHER;
        
        tinfo[i].sched_priority = atoi(priorities[i]);

        /* Set attributes to each thread, including CPU affinity and scheduling */
        pthread_attr_t attr;
        set_thread_attributes(&attr, tinfo[i].sched_policy, tinfo[i].sched_priority);
        
        /* Start the thread */
        pthread_create(&tinfo[i].thread_id, &attr, thread_func, &tinfo[i]);
        pthread_attr_destroy(&attr);
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(tinfo[i].thread_id, NULL);
    }

    /* Clean up */
    pthread_barrier_destroy(&barrier);
    free(tinfo);
    free(policies);
    free(priorities);

    return 0;
}
```

**Compile implemented program**
```
gcc -o sched_demo_312551002 sched_demo_312551002.c
```

**Test the program**
```
sudo ./sched_test.sh ./sched_demo ./sched_demo_312551002
```

**Screenshot of test result:**
![image](https://hackmd.io/_uploads/BkfCbABCT.png)


### Q2: Describe the results of ./sched_demo -n 3 -t 1.0 -s NORMAL,FIFO,FIFO -p -1,10,30 and what causes that.

![image](https://hackmd.io/_uploads/ByK0ZCHCp.png)

- **CFS and NORMAL Policy:** Thread 0 and Thread 2, which are both scheduled under the NORMAL policy, are managed by the CFS. This ensures they receive fair CPU time slices. Despite their lower priority compared to the FIFO threads, CFS allocates them CPU time in a manner that prevents starvation and ensures fair allocation. This is why these threads are scheduled intermittently even amidst the higher priority FIFO threads.
- **FIFO Policy and Preemption:** Threads 1 and 3, scheduled under the FIFO policy, are expected to run based on their priority levels. Thread 3, with the highest priority (30), should theoretically preempt other threads and run to completion first. However, the interleaved scheduling of Thread 0 and Thread 2 indicates that CFS intervention allows these NORMAL policy threads to receive CPU time, thus not strictly adhering to FIFO priorities.

### Q3: Describe the results of ./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30, and what causes that.

![image](https://hackmd.io/_uploads/H1Q1fCHRT.png)

- **CFS and NORMAL Policy:** Although Thread 0 and Thread 2 have a NORMAL scheduling policy, the CFS ensures they both receive a fair portion of CPU time. CFS is designed to equitably distribute CPU time slices and does not allow real-time processes to completely overshadow regular ones. This design accounts for why Threads 0 and 2 are scheduled intermittently, even though there are FIFO threads with higher priorities running.
- **FIFO Policy and Preemption:** For FIFO threads such as Thread 1 and Thread 3, the scheduling is based on their priority levels. Thread 3, with the highest priority (30), should, in a perfect FIFO system, run to completion before Thread 1, which has a lower priority (10), begins. However, the interleaving of Thread 0 and Thread 2 suggests that CFS is providing time slices to NORMAL policy threads, thereby not strictly adhering to FIFO priority rules.

### Q4: Describe how did you implement n-second-busy-waiting?
```c
void *thread_func(void *arg) {
    thread_info_t *tinfo = (thread_info_t *)arg;
    struct timespec start, current;
    
    /* Wait until all threads are ready */
    pthread_barrier_wait(tinfo->barrier);

    /* Do the task */
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", tinfo->thread_num);
        clock_gettime(CLOCK_MONOTONIC, &start);
        double elapsed = 0;

        /* Busy for <time_wait> seconds */
        while (elapsed < tinfo->time_wait) {
            clock_gettime(CLOCK_MONOTONIC, &current);
            elapsed = current.tv_sec - start.tv_sec + 
                     (current.tv_nsec - start.tv_nsec) / 1000000000.0;
        }
    }

    /* Exit the function  */
    return NULL;
}
```
- **Time Measurement:** The `clock_gettime` function with `CLOCK_MONOTONIC` is used to retrieve the current time at the start of the busy-waiting loop and then repeatedly during the loop to measure the elapsed time.
- **Busy-Wait Loop:** Each thread enters a while loop that continually checks whether the elapsed time since the start of the loop has exceeded the specified waiting time (time_wait).
- **Elapsed Time Calculation:** Inside the loop, clock_gettime is called again to get the current time, and the elapsed time is calculated by subtracting the starting time (start) from the current time (current). The result includes both the seconds (tv_sec) and nanoseconds (tv_nsec) components.