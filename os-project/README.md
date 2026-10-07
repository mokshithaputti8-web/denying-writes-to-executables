# Denying Writes to Executables

An Operating Systems and Systems Programming project showing how Linux
refuses to let a **running executable be modified** (`open()` fails with
`ETXTBSY` – "Text file busy"), and the OS concepts behind that rule.

Each source file is an independent program with its own plain output.

## Files

| File | What it shows | Main calls |
|------|---------------|-----------|
| `CO1_Kernel_Protection.c` | A program opens its own running binary for writing and is denied (user space → kernel space, `errno`) | `readlink`, `open` |
| `CO2_Process_Control.c`   | fork → exec → wait; parent is denied while the child runs `target_program` | `fork`, `execve`, `waitpid` |
| `CO3_IPC_Signals.c`       | Child tells parent "READY" via a pipe; parent is notified of exit by `SIGCHLD` | `pipe`, `sigaction`, `read/write` |
| `CO4_Virtual_Memory.c`    | Code pages are `r-xp`; writing → page fault → `SIGSEGV`; private mapping is copy-on-write | `mmap`, `/proc/self/maps`, `sigaction` |
| `CO5_File_System.c`       | Inodes, file I/O; write denied while running, allowed after exit; safe update with `rename()` (new inode) | `open`, `stat`, `lseek`, `rename`, `unlink` |
| `CO6_Thread_Sync.c`       | Race condition vs mutex; threads denied concurrently; semaphore + condition variable | `pthread_*`, `sem_*` |
| `target_program.c`        | Helper that runs for a few seconds (used by CO2 and CO5) | `getpid`, `sleep` |

## Requirements (Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential gdb
```

## Build and run (Ubuntu terminal)

```bash
cd Denying-Writes-to-Executables
make                 # build everything
./CO1_Kernel_Protection      # run any program on its own
make run             # or run all of them one after another
make clean           # remove binaries
```

## VS Code

1. Install VS Code and the extensions **C/C++** (`ms-vscode.cpptools`).
2. **Windows + WSL Ubuntu:** also install **WSL** (`ms-vscode-remote.remote-wsl`).
   Open the Ubuntu terminal, go to the project folder and run `code .`
3. Build: `Ctrl+Shift+B` (runs `make`).
4. Run: open the integrated terminal (`Ctrl+` `` ` ``) and run `./CO4_Virtual_Memory`, etc.,
   or use *Terminal → Run Task → Run all demos*.
5. Debug: open a `.c` file, press `F5` (builds, then starts gdb on that program).

### Important for WSL users
Keep the project inside the Linux file system (for example `~/projects/`),
**not** under `/mnt/c/...`. Files on the Windows drive may not enforce
`ETXTBSY`, so the "write denied" result may not appear there.

## Useful commands for the viva

```bash
strace -e trace=openat ./CO1_Kernel_Protection   # see open() fail with ETXTBSY
./target_program 20 &                            # run in background
echo x >> target_program                         # try to write -> "Text file busy"
cat /proc/$!/maps | head                         # memory map of that process
ls -i target_program; stat target_program        # inode information
valgrind ./CO3_IPC_Signals                       # (sudo apt install valgrind)
```

## Notes
- CO6 Part A: the unsynchronised counter may or may not lose updates on a
  given run (it depends on thread timing). The mutex version is always correct.
- Nothing is ever written to an executable; every demo only *attempts* to
  open it, then closes the descriptor.
