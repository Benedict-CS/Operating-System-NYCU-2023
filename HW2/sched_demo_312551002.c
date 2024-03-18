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