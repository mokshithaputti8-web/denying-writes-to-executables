/* =====================================================================
 * target_program.c
 * ---------------------------------------------------------------------
 * A small, harmless executable that the other programs launch with
 * fork() + execve() so they have a *running* executable to test.
 *
 * It prints its PID, "works" (sleeps) for a few seconds, then exits.
 * Usage:  ./target_program [seconds]     (default = 3 seconds)
 * ===================================================================== */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    unsigned int seconds = 3;
    if (argc > 1) {
        int s = atoi(argv[1]);
        if (s > 0 && s < 60) seconds = (unsigned int)s;
    }

    printf("[target_program] Hello! My PID is %d\n", getpid());
    printf("[target_program] I am now running (for %u seconds)...\n", seconds);
    fflush(stdout);

    sleep(seconds);

    printf("[target_program] Work finished. Exiting normally now.\n");
    fflush(stdout);
    return 0;
}
