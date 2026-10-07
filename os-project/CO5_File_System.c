/* =====================================================================
 * CO5_File_System.c
 * ---------------------------------------------------------------------
 * File systems and file I/O: inodes, file descriptors, and why a
 * running executable cannot be written to -- but CAN be replaced.
 *
 * Steps:
 *   1. Make a private copy of target_program (open/read/write/close).
 *   2. Inspect the copy: inode number, size, permissions, link count
 *      (stat) and its ELF header (open/read/lseek).
 *   3. Run the copy with fork()+execve(). While it runs, try
 *      open(O_WRONLY) -> ETXTBSY, but open(O_RDONLY) still works.
 *   4. After the process exits, open(O_WRONLY) works again.
 *   5. Start it again and REPLACE the file with rename(). This is
 *      allowed: the new file has a NEW inode, while the running
 *      process keeps using the OLD inode. This is how package
 *      managers update programs that are in use.
 *
 * System calls: open, read, write, lseek, close, fstat, stat, rename,
 *               unlink, fork, execve, waitpid
 * ===================================================================== */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SOURCE_PATH "./target_program"
#define COPY_PATH   "./demo_copy"
#define NEW_PATH    "./demo_copy.new"

/* Copy a file using low-level file I/O. Returns 0 on success. */
static int copy_file(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    if (in == -1) { perror("open source (run 'make' first)"); return -1; }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out == -1) { perror("open destination"); close(in); return -1; }

    char buf[4096];
    ssize_t n;
    while ((n = read(in, buf, sizeof buf)) > 0) {
        if (write(out, buf, (size_t)n) != n) {
            perror("write"); close(in); close(out); return -1;
        }
    }
    close(in);
    close(out);              /* must be closed before exec, or exec -> ETXTBSY */
    return (n < 0) ? -1 : 0;
}

static void show_info(const char *label, const char *path)
{
    struct stat st;
    if (stat(path, &st) == -1) { perror("stat"); return; }
    printf("   %-22s inode = %-9lu size = %ld bytes  mode = %o  links = %lu\n",
           label, (unsigned long)st.st_ino, (long)st.st_size,
           st.st_mode & 0777, (unsigned long)st.st_nlink);
}

static pid_t launch(const char *path, const char *secs)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        char *args[] = { (char *)path, (char *)secs, NULL };
        char *envp[] = { NULL };
        execve(path, args, envp);
        perror("execve");
        _exit(EXIT_FAILURE);
    }
    return pid;
}

static void try_open(const char *path, int flags, const char *what)
{
    int fd = open(path, flags);
    if (fd == -1) {
        printf("   %-26s -> DENIED  (errno %d: %s)\n", what, errno, strerror(errno));
    } else {
        printf("   %-26s -> allowed (fd = %d)\n", what, fd);
        close(fd);           /* nothing is written */
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);   /* keep output in order even when piped */

    printf("=====================================================\n");
    printf(" File System Demo: Inodes, File I/O and Safe Updates\n");
    printf("=====================================================\n\n");

    /* ---------------- Step 1 ---------------- */
    printf("Step 1: Creating a working copy of target_program\n");
    if (copy_file(SOURCE_PATH, COPY_PATH) == -1) return EXIT_FAILURE;
    printf("   Copied %s -> %s\n\n", SOURCE_PATH, COPY_PATH);

    /* ---------------- Step 2 ---------------- */
    printf("Step 2: File metadata and header\n");
    show_info("demo_copy (inode table):", COPY_PATH);

    int fd = open(COPY_PATH, O_RDONLY);
    if (fd != -1) {
        unsigned char magic[4];
        lseek(fd, 0, SEEK_SET);
        if (read(fd, magic, 4) == 4)
            printf("   First 4 bytes: %02x %c %c %c  (ELF executable signature)\n",
                   magic[0], magic[1], magic[2], magic[3]);
        printf("   File descriptor used for reading: %d\n", fd);
        close(fd);
    }
    printf("\n");

    /* ---------------- Step 3 ---------------- */
    printf("Step 3: Opening the file while it is RUNNING\n");
    pid_t pid = launch(COPY_PATH, "3");
    usleep(500000);
    printf("   Process %d is executing %s\n", pid, COPY_PATH);
    try_open(COPY_PATH, O_WRONLY, "open(O_WRONLY)");
    try_open(COPY_PATH, O_RDONLY, "open(O_RDONLY)");
    waitpid(pid, NULL, 0);
    printf("\n");

    /* ---------------- Step 4 ---------------- */
    printf("Step 4: Opening the file after the process has exited\n");
    try_open(COPY_PATH, O_WRONLY, "open(O_WRONLY)");
    printf("   -> The 'text file busy' lock was released when the process ended.\n\n");

    /* ---------------- Step 5 ---------------- */
    printf("Step 5: Replacing a running executable with rename()\n");
    pid = launch(COPY_PATH, "3");
    usleep(500000);
    show_info("old file (running):", COPY_PATH);

    if (copy_file(SOURCE_PATH, NEW_PATH) == 0 && rename(NEW_PATH, COPY_PATH) == 0) {
        show_info("new file (same name):", COPY_PATH);
        printf("   rename() succeeded: the name now points to a different inode.\n");
        printf("   The running process still uses the old inode until it exits,\n");
        printf("   so no running code was ever modified.\n");
    } else {
        perror("replace");
    }
    waitpid(pid, NULL, 0);

    unlink(COPY_PATH);       /* clean up the demo file */
    printf("\n   Demo file removed with unlink().\n");

    printf("\n=====================================================\n");
    printf(" Demonstration complete.\n");
    printf("=====================================================\n");
    return 0;
}
