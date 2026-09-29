/* =====================================================================
 * target_program.c
 * ---------------------------------------------------------------------
 * This is a small, harmless executable that is launched by:
 *      - CO2_Process_Control.c   (using fork + execve)
 *      - CO3_IPC_Signals.c       (using fork + execve)
 *
 * It simply:
 *      1. Prints its own Process ID (PID)
 *      2. Announces that it is running
 *      3. Sleeps for a few seconds (simulating some "work")
 *      4. Exits normally
 *
 * Because this program keeps running for a few seconds, it is also
 * useful to manually test CO1 (try opening it for writing from another
 * terminal while it runs, and you will get ETXTBSY).
 * =====================================================================
 */

#define _POSIX_C_SOURCE 200809L  /* needed so pid_t/sleep() are visible with -std=c99 */

#include <stdio.h>
#include <unistd.h>    /* for getpid(), sleep() */
#include <sys/types.h> /* for pid_t */

int main(void)
{
    pid_t my_pid = getpid();

    printf("[target_program] Hello! My PID is %d\n", my_pid);
    printf("[target_program] I am now running...\n");
    fflush(stdout);   /* make sure output is printed immediately */

    /* Simulate doing some work for 3 seconds */
    sleep(3);

    printf("[target_program] Work finished. Exiting normally now.\n");
    fflush(stdout);

    return 0;
}
