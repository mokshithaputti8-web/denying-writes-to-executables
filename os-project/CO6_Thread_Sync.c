/* =====================================================================
 * CO6_Thread_Sync.c
 * ---------------------------------------------------------------------
 * Concurrency and synchronization with POSIX threads.
 *
 * Part A  Race condition: several threads increment one shared counter.
 *         Without a lock updates are lost; with a mutex the total is
 *         always correct.
 *
 * Part B  Several threads at the same time try to open the running
 *         executable for writing. Every attempt is denied (ETXTBSY).
 *           - a counting semaphore limits how many threads attempt
 *             the open() at once,
 *           - a mutex protects the shared "denied" counter,
 *           - a condition variable lets main() sleep until every
 *             worker is finished (no busy waiting).
 *
 * Build needs -pthread (the Makefile already does this).
 * Calls: pthread_create/join, pthread_mutex_*, pthread_cond_*, sem_*
 * ===================================================================== */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

/* ----------------------------- Part A ------------------------------ */
#define THREADS_A      4
#define INCREMENTS     200000L

static long unsafe_counter = 0;
static long safe_counter   = 0;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

static void *increment_unsafe(void *arg)
{
    (void)arg;
    for (long i = 0; i < INCREMENTS; i++) {
        long tmp = unsafe_counter;            /* 1. read                      */
        if (i % 1000 == 0)
            sched_yield();                    /* let another thread interleave */
        unsafe_counter = tmp + 1;             /* 2. write: NOT atomic overall */
    }
    return NULL;
}

static void *increment_safe(void *arg)
{
    (void)arg;
    for (long i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&counter_lock);
        safe_counter++;                       /* protected critical section */
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

static void run_counter_test(void *(*fn)(void *))
{
    pthread_t t[THREADS_A];
    for (int i = 0; i < THREADS_A; i++) pthread_create(&t[i], NULL, fn, NULL);
    for (int i = 0; i < THREADS_A; i++) pthread_join(t[i], NULL);
}

/* ----------------------------- Part B ------------------------------ */
#define WORKERS 6

static char exe_path[1024];
static int  results[WORKERS];                 /* errno seen by each worker */
static int  denied_count = 0;
static int  finished     = 0;

static pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  all_done   = PTHREAD_COND_INITIALIZER;
static sem_t           slots;                 /* counting semaphore */

static void *worker(void *arg)
{
    int id = (int)(long)arg;

    sem_wait(&slots);                         /* at most 2 workers inside */
    int fd = open(exe_path, O_WRONLY);
    int err = (fd == -1) ? errno : 0;
    if (fd != -1) close(fd);                  /* never writes anything */
    usleep(100000);                           /* pretend to do more work */
    sem_post(&slots);

    pthread_mutex_lock(&state_lock);
    results[id] = err;
    if (err == ETXTBSY) denied_count++;
    finished++;
    if (finished == WORKERS)
        pthread_cond_signal(&all_done);       /* wake up main() */
    pthread_mutex_unlock(&state_lock);
    return NULL;
}

int main(void)
{
    printf("=====================================================\n");
    printf(" Thread Synchronization Demo\n");
    printf("=====================================================\n\n");

    /* ---------------- Part A ---------------- */
    long expected = (long)THREADS_A * INCREMENTS;
    printf("Part A: %d threads x %ld increments on one shared counter\n",
           THREADS_A, INCREMENTS);
    printf("   Expected final value : %ld\n", expected);

    run_counter_test(increment_unsafe);
    printf("   Without a mutex      : %ld  %s\n", unsafe_counter,
           unsafe_counter == expected ? "(no lost update this run - run again)"
                                      : "<- updates were lost (race condition)");

    run_counter_test(increment_safe);
    printf("   With a mutex         : %ld  %s\n\n", safe_counter,
           safe_counter == expected ? "<- correct" : "<- unexpected");

    /* ---------------- Part B ---------------- */
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof exe_path - 1);
    if (len == -1) { perror("readlink"); return EXIT_FAILURE; }
    exe_path[len] = '\0';

    printf("Part B: %d threads try to open the running executable for writing\n", WORKERS);
    printf("   Target: %s\n", exe_path);
    printf("   Semaphore allows 2 threads to attempt at the same time.\n\n");

    sem_init(&slots, 0, 2);

    pthread_t t[WORKERS];
    for (long i = 0; i < WORKERS; i++)
        pthread_create(&t[i], NULL, worker, (void *)i);

    /* Sleep until the last worker signals the condition variable. */
    pthread_mutex_lock(&state_lock);
    while (finished < WORKERS)
        pthread_cond_wait(&all_done, &state_lock);
    pthread_mutex_unlock(&state_lock);

    for (int i = 0; i < WORKERS; i++) pthread_join(t[i], NULL);

    for (int i = 0; i < WORKERS; i++)
        printf("   Thread %d: open(O_WRONLY) -> %s\n", i + 1,
               results[i] == 0 ? "allowed" :
               results[i] == ETXTBSY ? "DENIED (ETXTBSY: Text file busy)" : strerror(results[i]));

    printf("\n   Writes denied: %d out of %d\n", denied_count, WORKERS);

    sem_destroy(&slots);
    pthread_mutex_destroy(&state_lock);
    pthread_mutex_destroy(&counter_lock);
    pthread_cond_destroy(&all_done);

    printf("\n=====================================================\n");
    printf(" Demonstration complete.\n");
    printf("=====================================================\n");
    return 0;
}
