/* =====================================================================
 * CO3_IPC_Signals.c
 * ---------------------------------------------------------------------
 * COURSE OUTCOME 3 (CO-3): Inter-Process Communication (IPC) and Signals
 *
 * CONCEPT DEMONSTRATED:
 * ----------------------
 *      Child process
 *            |
 *          pipe()                 -> an anonymous, one-way IPC channel
 *            |                       is created before forking
 *      Send "READY" message  ----->  written by child into the pipe
 *            |
 *      Parent receives "READY"     -> parent reads the message
 *            |
 *      Child terminates
 *            |
 *          SIGCHLD                -> kernel sends this signal to the
 *            |                       parent automatically
 *      Parent handles termination -> a signal handler function runs
 *
 * SYSTEM CALLS / CONCEPTS USED:
 *      - pipe()               : creates an anonymous pipe (two file
 *                                descriptors: read end and write end)
 *      - fork()                : creates the child process
 *      - read() / write()      : used to send data through the pipe
 *      - sigaction()            : installs a signal handler for SIGCHLD
 *      - SIGCHLD                : signal automatically sent to a parent
 *                                 process when one of its children
 *                                 terminates (asynchronous notification)
 *      - waitpid() (in handler) : reaps the terminated child safely
 *
 * This program shows TWO different ways a child can communicate
 * information to its parent:
 *      1. Synchronously through a pipe ("READY" message)
 *      2. Asynchronously through a signal (SIGCHLD on termination)
 * =====================================================================
 */

#define _POSIX_C_SOURCE 200809L  /* needed for sigaction()/SA_RESTART with -std=c99 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/* This flag is set by the signal handler when SIGCHLD arrives.
 * It is declared 'volatile sig_atomic_t' because it is modified
 * inside a signal handler (async-signal-safe practice). */
volatile sig_atomic_t child_terminated = 0;

/* ---------------------------------------------------------------------
 * Signal handler for SIGCHLD.
 * This function runs ASYNCHRONOUSLY -- the kernel invokes it whenever
 * a child process of ours terminates, interrupting whatever the parent
 * was doing at that moment.
 * -------------------------------------------------------------------*/
void sigchld_handler(int signum)
{
    (void)signum;  /* unused parameter */

    /* Reap the child to prevent a zombie process.
     * We keep this handler simple and safe for a college project. */
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        /* loop in case multiple children exited */
    }
    errno = saved_errno;

    child_terminated = 1;

    /* NOTE: printf() is not strictly "async-signal-safe", but for a
     * simple educational demonstration on Linux it works reliably and
     * makes the output easy to understand. */
    printf("[Parent] >>> SIGCHLD received! Kernel notified us that the "
           "child has terminated. <<<\n");
}

int main(void)
{
    int pipe_fd[2];   /* pipe_fd[0] = read end, pipe_fd[1] = write end */
    char buffer[100];

    printf("=====================================================\n");
    printf(" CO3: IPC (pipe) and Signals (SIGCHLD)\n");
    printf("=====================================================\n\n");

    /* -----------------------------------------------------------
     * STEP 1: Create the pipe BEFORE forking, so that both parent
     * and child inherit the same pipe file descriptors.
     * -----------------------------------------------------------
     */
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }
    printf("[Parent] Pipe created. Read end = %d, Write end = %d\n",
           pipe_fd[0], pipe_fd[1]);

    /* -----------------------------------------------------------
     * STEP 2: Install the SIGCHLD signal handler BEFORE forking,
     * so the parent is ready to catch the signal at any time.
     * -----------------------------------------------------------
     */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;   /* restart interrupted system calls */

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }
    printf("[Parent] SIGCHLD handler installed.\n\n");

    /* IMPORTANT: flush stdout BEFORE fork().
     * printf() output is line-buffered/block-buffered in memory before
     * it is actually written out. If we fork() while text is still
     * sitting in that buffer, BOTH the parent and the child inherit a
     * copy of the buffer and each of them will print it again later,
     * causing confusing duplicate output. Flushing first avoids this. */
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        /* --------------------- CHILD PROCESS ------------------------ */

        /* The child only needs the WRITE end of the pipe, so close
         * the unused READ end. */
        close(pipe_fd[0]);

        printf("[Child ] My PID is %d. Sending READY message through "
               "the pipe...\n", getpid());

        const char *msg = "READY";
        write(pipe_fd[1], msg, strlen(msg) + 1);

        close(pipe_fd[1]);   /* done writing */

        printf("[Child ] Message sent. Now doing a little work and "
               "then exiting...\n");
        sleep(2);

        printf("[Child ] Terminating now (this will trigger SIGCHLD "
               "in the parent).\n");
        exit(0);
    }
    else
    {
        /* --------------------- PARENT PROCESS ------------------------ */

        /* The parent only needs the READ end of the pipe, so close
         * the unused WRITE end. */
        close(pipe_fd[1]);

        printf("[Parent] Child process created with PID = %d\n", pid);
        printf("[Parent] Waiting to receive message from child via "
               "pipe...\n");

        ssize_t n = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            printf("[Parent] Received message from child: \"%s\"\n\n",
                   buffer);
        }

        close(pipe_fd[0]);

        printf("[Parent] Now waiting for asynchronous SIGCHLD "
               "notification when the child terminates...\n");

        /* The parent can keep doing other things here. We simply
         * pause/sleep until the SIGCHLD handler sets the flag,
         * demonstrating that the notification is asynchronous. */
        while (!child_terminated)
        {
            printf("[Parent] (still waiting... doing other work)\n");
            sleep(1);
        }

        printf("\n[Parent] Confirmed: child process has fully "
               "terminated and was reaped.\n");

        printf("\n=====================================================\n");
        printf(" CO3 demonstration complete.\n");
        printf("=====================================================\n");
    }

    return 0;
}
