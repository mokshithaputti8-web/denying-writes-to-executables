/*
 * =====================================================================
 *  CO2_Process_Control.c
 * =====================================================================
 *
 *  COURSE OUTCOME 2 (CO2): Process Control
 *
 *  CONCEPT DEMONSTRATED:
 *  ----------------------
 *  The classic Unix/Linux process life cycle:
 *
 *      Parent process
 *            |
 *          fork()               -> creates a new (child) process
 *            |
 *      Child process created
 *            |
 *          execve()              -> child replaces itself with a new program
 *            |
 *      target_program runs
 *            |
 *      target_program exits
 *            |
 *      Parent waits using waitpid()
 *
 *  This program is the "control" process: it creates a child, makes
 *  that child become target_program, and then waits for it to finish,
 *  printing clear status messages (with PIDs) at every stage.
 *
 *  SYSTEM CALLS / CONCEPTS USED:
 *      fork(), execve(), waitpid(), getpid(), exit status handling
 * =====================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define TARGET_PATH "./target_program"

int main(void)
{
    printf("=====================================================\n");
    printf(" CO2 DEMO: Process Control (fork -> exec -> wait)\n");
    printf("=====================================================\n\n");

    printf("[Parent] I am the parent process. My PID = %d\n", getpid());
    printf("[Parent] About to create a child process using fork()...\n\n");
    fflush(stdout);   /* flush before fork() so buffered text is not
                          duplicated by the child (important when output
                          is redirected to a file/pipe, not a terminal) */

    /* -----------------------------------------------------------
     * STEP 1: fork() - create a new child process.
     * ----------------------------------------------------------- */
    pid_t pid = fork();

    if (pid < 0) {
        /* fork() failed */
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* -------------------------------------------------------
         * STEP 2: CHILD PROCESS
         * ------------------------------------------------------- */
        printf("[Child ] Child process created successfully! My PID = %d\n", getpid());
        printf("[Child ] Parent's PID (from child's view) = %d\n", getppid());
        printf("[Child ] Now replacing myself with target_program using execve()...\n\n");

        char *args[] = { TARGET_PATH, NULL };
        char *envp[] = { NULL };

        /* execve() replaces the child's memory image with target_program.
           If successful, the code below execve() never runs. */
        execve(TARGET_PATH, args, envp);

        /* If we reach here, execve() failed. */
        perror("execve failed (did you run 'make' first?)");
        exit(EXIT_FAILURE);
    }

    /* -------------------------------------------------------
     * STEP 3: PARENT PROCESS continues here.
     * ------------------------------------------------------- */
    printf("[Parent] Child process launched with PID = %d\n", pid);
    printf("[Parent] Waiting for child (target_program) to terminate...\n\n");

    int status;
    pid_t finished_pid = waitpid(pid, &status, 0);

    printf("\n[Parent] wait() returned. Child PID %d has terminated.\n", finished_pid);

    /* -----------------------------------------------------------
     * STEP 4: Inspect and report the child's exit status.
     * ----------------------------------------------------------- */
    if (WIFEXITED(status)) {
        printf("[Parent] target_program exited normally with exit code %d.\n",
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[Parent] target_program was terminated by signal %d.\n",
               WTERMSIG(status));
    } else {
        printf("[Parent] target_program terminated abnormally.\n");
    }

    printf("\n[Parent] Process life cycle complete:\n");
    printf("   Parent -> fork() -> Child created -> exec() ->\n");
    printf("   target_program ran -> target_program exited ->\n");
    printf("   Parent detected termination via waitpid().\n");

    return 0;
}
