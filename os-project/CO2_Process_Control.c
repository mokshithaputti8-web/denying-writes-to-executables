/* =====================================================================
 * CO2_Process_Control.c
 * ---------------------------------------------------------------------
 * Process control: fork() -> execve() -> waitpid()
 *
 *   Parent --fork()--> Child --execve()--> target_program runs
 *   Parent waits with waitpid() and reads the exit status.
 *
 * Link to the project theme: while the child is running
 * target_program, the parent tries to open that same file for writing.
 * The kernel answers with ETXTBSY, because the file is now the
 * executable image of a live process.
 *
 * System calls: fork, execve, waitpid, getpid, getppid, open, usleep
 * ===================================================================== */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TARGET_PATH "./target_program"

int main(void)
{
    printf("=====================================================\n");
    printf(" Process Control Demo: fork -> exec -> wait\n");
    printf("=====================================================\n\n");

    printf("[Parent] My PID = %d\n", getpid());
    printf("[Parent] Creating a child process using fork()...\n\n");
    fflush(stdout);   /* avoid duplicated buffered output in the child */

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* ------------------------- CHILD ------------------------- */
        printf("[Child ] Created. My PID = %d, my parent's PID = %d\n",
               getpid(), getppid());
        printf("[Child ] Replacing myself with target_program using execve()...\n\n");
        fflush(stdout);

        char *args[] = { TARGET_PATH, NULL };
        char *envp[] = { NULL };
        execve(TARGET_PATH, args, envp);

        perror("execve failed (did you run 'make' first?)");
        _exit(EXIT_FAILURE);
    }

    /* --------------------------- PARENT --------------------------- */
    printf("[Parent] Child launched with PID = %d\n", pid);

    usleep(500000);   /* give the child time to finish execve() */

    printf("[Parent] While the child runs, I try to open %s for writing...\n",
           TARGET_PATH);
    int fd = open(TARGET_PATH, O_WRONLY);
    if (fd == -1) {
        printf("[Parent] WRITE DENIED -> errno = %d (%s)\n", errno, strerror(errno));
        if (errno == ETXTBSY)
            printf("[Parent] The kernel protects the running executable.\n");
    } else {
        printf("[Parent] open() succeeded (nothing is written). Closing it.\n");
        close(fd);
    }

    printf("[Parent] Waiting for the child to terminate...\n\n");
    fflush(stdout);

    int status;
    pid_t done = waitpid(pid, &status, 0);

    printf("\n[Parent] waitpid() returned: child %d has terminated.\n", done);
    if (WIFEXITED(status))
        printf("[Parent] Child exited normally with exit code %d.\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("[Parent] Child was killed by signal %d.\n", WTERMSIG(status));

    printf("\n=====================================================\n");
    printf(" Demonstration complete.\n");
    printf("=====================================================\n");
    return 0;
}
