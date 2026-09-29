/* =====================================================================
 * CO1_Kernel_Protection.c
 * ---------------------------------------------------------------------
 * COURSE OUTCOME 1 (CO-1): Operating System Concepts, System Calls,
 *                          Kernel Space vs User Space
 *
 * CONCEPT DEMONSTRATED:
 * ----------------------
 * The Linux kernel does NOT allow a process to open a currently
 * EXECUTING binary file in write mode. If you try to do so, the
 * open() system call fails and sets errno to ETXTBSY
 * ("Text file busy").
 *
 * This is a real kernel-level protection mechanism: while a program's
 * code (its "text segment") is loaded into memory and running, the
 * kernel marks that file as busy so that no one can modify the bytes
 * of a program while the CPU is actively executing them. This keeps
 * the system stable and prevents corruption of running code.
 *
 * HOW THIS PROGRAM DEMONSTRATES IT SAFELY:
 * ------------------------------------------
 * This program finds the path of its OWN executable file (the very
 * file that is currently running as this process) using the special
 * Linux symlink /proc/self/exe. Then, while it is still running, it
 * calls open() on that same path with O_WRONLY (write-only) mode.
 *
 * Since this program itself is the "currently running executable",
 * the kernel will refuse the request and open() will return -1 with
 * errno set to ETXTBSY. We never actually write anything to the file,
 * so nothing is corrupted -- we only PROVE that the kernel blocked us.
 *
 * SYSTEM CALLS / CONCEPTS USED:
 *      - open()            : user-space request that crosses into
 *                             kernel space (a system call)
 *      - file descriptors   : what open() would normally return
 *      - errno              : how the kernel reports failure reasons
 *                             back to user space
 *      - ETXTBSY            : the specific error code for this
 *                             protection mechanism
 *      - readlink()          : used to discover our own executable path
 * =====================================================================
 */

#define _DEFAULT_SOURCE  /* needed so readlink() is visible with -std=c99 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>      /* open(), O_WRONLY */
#include <errno.h>      /* errno, ETXTBSY   */
#include <string.h>

int main(void)
{
    char exe_path[1024];
    ssize_t len;

    printf("=====================================================\n");
    printf(" CO1: Kernel Protection Against Writing to Executables\n");
    printf("=====================================================\n\n");

    printf("Step 1: Discovering the path of my own running executable...\n");

    /*
     * /proc/self/exe is a special symbolic link maintained by the
     * Linux kernel. It always points to the executable file of the
     * process that is reading it (i.e., THIS program).
     */
    len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
    {
        perror("readlink failed");
        exit(EXIT_FAILURE);
    }
    exe_path[len] = '\0';   /* readlink does not null-terminate */

    printf("   -> My executable path is: %s\n", exe_path);
    printf("   -> My Process ID (PID) is: %d\n\n", getpid());

    printf("Step 2: Attempting to open my own executable for WRITING\n");
    printf("        while it is currently running...\n\n");

    printf("Attempting to open executable for writing...\n");

    /*
     * THE IMPORTANT SYSTEM CALL:
     * We ask the kernel to open our own running binary in write mode.
     * Because this file is presently being executed (its text segment
     * is mapped and active), the kernel is expected to deny this.
     */
    int fd = open(exe_path, O_WRONLY);

    if (fd == -1)
    {
        /* The open() system call failed. Inspect errno to find out why. */
        printf("WRITE DENIED\n");
        printf("errno = %d\n", errno);

        if (errno == ETXTBSY)
        {
            printf("ETXTBSY: Text file busy\n");
            printf("Linux kernel prevented the write.\n\n");
            printf("EXPLANATION:\n");
            printf("The kernel refused to let us open this executable\n");
            printf("for writing because it is currently being executed\n");
            printf("as a running process. This protects the integrity\n");
            printf("of running programs and demonstrates how the kernel\n");
            printf("enforces boundaries between user-space requests and\n");
            printf("protected system resources.\n");
        }
        else
        {
            /* In case some other error occurred, still show it clearly */
            printf("(Unexpected error) %s\n", strerror(errno));
        }
    }
    else
    {
        /*
         * If we somehow succeeded (should not normally happen while
         * running), we safely close the descriptor without writing
         * any data, so the executable is never corrupted.
         */
        printf("Unexpectedly succeeded in opening the file for write.\n");
        printf("No data will be written -- closing file descriptor %d now.\n", fd);
        close(fd);
    }

    printf("\n=====================================================\n");
    printf(" CO1 demonstration complete.\n");
    printf("=====================================================\n");

    return 0;
}
