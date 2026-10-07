/* =====================================================================
 * CO4_Virtual_Memory.c
 * ---------------------------------------------------------------------
 * Virtual memory: how the MMU/kernel protect program code.
 *
 * A running executable is mapped into the process's virtual address
 * space as a READ + EXECUTE region (r-xp) -- never writable. This
 * program shows that protection from the memory side:
 *
 *   Step 1  Show the executable's own mappings in /proc/self/maps.
 *   Step 2  Map the executable file read-only and try to write to it
 *           -> the MMU raises a page fault, the kernel sends SIGSEGV.
 *           We catch it with sigaction + siglongjmp and carry on.
 *   Step 3  mmap(PROT_WRITE, MAP_SHARED) on a read-only descriptor
 *           -> the kernel refuses (EACCES): a shared writable mapping
 *           would modify the file on disk.
 *   Step 4  mmap(PROT_WRITE, MAP_PRIVATE): allowed, but copy-on-write.
 *           The write goes to a private copy of the page; the file on
 *           disk stays unchanged (verified with pread()).
 *
 * Concepts: virtual address space, page permissions, page fault,
 *           SIGSEGV, copy-on-write, mmap, /proc/self/maps
 * ===================================================================== */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/stat.h>

static sigjmp_buf jump_point;
static volatile sig_atomic_t caught_segv = 0;

static void segv_handler(int signum)
{
    (void)signum;
    caught_segv = 1;
    siglongjmp(jump_point, 1);   /* leave the faulting instruction */
}

static void print_own_mappings(const char *exe_path)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("fopen /proc/self/maps"); return; }

    char line[512];
    printf("   %-34s %-5s %s\n", "address range", "perms", "backing");
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, exe_path) || strstr(line, "[stack]") || strstr(line, "[heap]")) {
            char range[64], perms[8], off[32], dev[16], inode[32], path[400] = "";
            if (sscanf(line, "%63s %7s %31s %15s %31s %399[^\n]",
                       range, perms, off, dev, inode, path) >= 5)
                printf("   %-34s %-5s %s\n", range, perms, path[0] ? path : "[anonymous]");
        }
    }
    fclose(f);
}

int main(void)
{
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof exe_path - 1);
    if (len == -1) { perror("readlink"); return EXIT_FAILURE; }
    exe_path[len] = '\0';

    printf("=====================================================\n");
    printf(" Virtual Memory Demo: Protected Executable Mappings\n");
    printf("=====================================================\n\n");

    long page = sysconf(_SC_PAGESIZE);
    printf("Page size on this system: %ld bytes\n", page);
    printf("Executable: %s\n\n", exe_path);

    /* ---------------- Step 1 ---------------- */
    printf("Step 1: Memory map of this running process\n");
    print_own_mappings(exe_path);
    printf("   -> The code segment is 'r-xp': readable + executable,\n");
    printf("      but NOT writable.\n\n");

    int fd = open(exe_path, O_RDONLY);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }

    /* ---------------- Step 2 ---------------- */
    printf("Step 2: Write into a read-only mapping of the executable\n");
    unsigned char *ro = mmap(NULL, (size_t)page, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ro == MAP_FAILED) { perror("mmap"); return EXIT_FAILURE; }
    printf("   Mapped first page at %p (PROT_READ). First byte = 0x%02x\n", (void *)ro, ro[0]);

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);

    printf("   Attempting to write to it...\n");
    fflush(stdout);
    if (sigsetjmp(jump_point, 1) == 0) {
        ro[0] = 0x00;                         /* page fault -> SIGSEGV */
        printf("   (unexpected) write succeeded\n");
    }
    if (caught_segv)
        printf("   WRITE DENIED: page fault -> SIGSEGV caught.\n"
               "   The MMU blocked the write; the kernel reported it as a signal.\n\n");
    signal(SIGSEGV, SIG_DFL);
    munmap(ro, (size_t)page);

    /* ---------------- Step 3 ---------------- */
    printf("Step 3: Shared writable mapping on a read-only file descriptor\n");
    void *sh = mmap(NULL, (size_t)page, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        printf("   WRITE DENIED: mmap() failed, errno = %d (%s)\n\n", errno, strerror(errno));
    } else {
        printf("   (unexpected) mapping created\n\n");
        munmap(sh, (size_t)page);
    }

    /* ---------------- Step 4 ---------------- */
    printf("Step 4: Private writable mapping (copy-on-write)\n");
    unsigned char *priv = mmap(NULL, (size_t)page, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE, fd, 0);
    if (priv == MAP_FAILED) { perror("mmap private"); close(fd); return EXIT_FAILURE; }

    unsigned char before_map = priv[0];
    priv[0] = 0xFF;                           /* triggers a COW page copy */
    unsigned char disk_byte = 0;
    if (pread(fd, &disk_byte, 1, 0) != 1) perror("pread");

    printf("   Byte 0 in memory before write : 0x%02x\n", before_map);
    printf("   Byte 0 in memory after write  : 0x%02x  (private copy)\n", priv[0]);
    printf("   Byte 0 of the file on disk    : 0x%02x  (unchanged)\n", disk_byte);
    printf("   -> The kernel copied the page on first write, so the\n");
    printf("      executable file itself was never modified.\n");

    munmap(priv, (size_t)page);
    close(fd);

    printf("\n=====================================================\n");
    printf(" Demonstration complete.\n");
    printf("=====================================================\n");
    return 0;
}
