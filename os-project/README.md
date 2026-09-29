# Denying Writes to Executables

An Operating Systems / Systems Programming mini-project demonstrating how
the Linux kernel protects a currently running executable from being
modified, and how processes are created, communicate, and are notified
of each other's termination.

---

## 1. Project Title

**Denying Writes to Executables** — A Study of Kernel-Level Protection,
Process Control, and Inter-Process Communication in Linux.

---

## 2. Problem Statement

When a program is running on Linux, its executable file is loaded into
memory and actively used by the CPU. If another process (or even itself)
were allowed to freely modify that file on disk while it is executing,
the running program's behavior could become corrupted or unpredictable,
potentially crashing the system or causing security issues.

This project investigates **how the operating system prevents this
scenario**, and, more broadly, demonstrates the surrounding OS concepts
needed to fully understand *why* and *how* this protection fits into
the bigger picture of process management in Linux.

---

## 3. Objective

1. Demonstrate that Linux denies write access to a currently executing
   binary, using the `ETXTBSY` ("Text file busy") error, and understand
   the system-call-level interaction between user space and kernel
   space that causes this.
2. Demonstrate the complete lifecycle of a process — creation with
   `fork()`, program replacement with `execve()`, and termination
   handling with `wait()`/`waitpid()`.
3. Demonstrate inter-process communication using an anonymous `pipe()`
   and asynchronous notification of process termination using the
   `SIGCHLD` signal.

Together, these three parts build a full picture of how the OS manages
running programs — from protecting them while they run, to creating
and destroying them, to letting processes talk to each other.

---

## 4. CO-1 — Kernel Protection (`CO1_Kernel_Protection.c`)

**Course Outcome covered:** Operating System Concepts, System Calls,
Kernel Space vs. User Space.

This program:
- Finds the path of its own running executable using the
  `/proc/self/exe` symlink (via `readlink()`).
- Calls `open()` on that same file with `O_WRONLY` **while it is still
  running**.
- The Linux kernel refuses this request because the file's text
  segment is currently mapped and executing.
- `open()` returns `-1` and sets `errno` to `ETXTBSY`.
- The program prints the exact `errno` value and a clear explanation,
  **without ever writing a single byte** to the executable — so
  nothing is corrupted. This is a **safe, real demonstration**, not a
  simulation.

This shows the boundary between **user space** (our program requesting
an operation) and **kernel space** (the OS deciding whether to allow
it), and how system calls report failures back via `errno`.

---

## 5. CO-2 — Process Control (`CO2_Process_Control.c`)

**Course Outcome covered:** Process Control (creation, execution,
termination).

This program demonstrates the classic UNIX process lifecycle:

```
Parent process
      |
   fork()
      |
Child process created
      |
   execve()
      |
target_program runs
      |
target_program exits
      |
Parent waits using waitpid()
```

- `fork()` creates a new child process (an almost-exact copy of the
  parent).
- The child calls `execve()` to completely replace itself with a new
  program: `target_program`.
- The parent calls `waitpid()` to block until the child terminates,
  then inspects the exit status using `WIFEXITED` / `WEXITSTATUS`.
- Every step prints the relevant **PID** so you can see the parent and
  child are genuinely two separate processes.

---

## 6. CO-3 — IPC and Signals (`CO3_IPC_Signals.c`)

**Course Outcome covered:** Inter-Process Communication and Signals.

This program demonstrates two communication mechanisms between a
parent and its child:

```
Child process
     |
  pipe()
     |
Send "READY"  ---->  Parent receives "READY"
     |
Child terminates
     |
  SIGCHLD
     |
Parent handles termination (signal handler)
```

- An anonymous `pipe()` is created **before** `fork()`, so both parent
  and child share it.
- The child writes the message `"READY"` into the pipe; the parent
  reads it with `read()` — this is **synchronous** IPC.
- A `SIGCHLD` handler is installed with `sigaction()` **before**
  forking.
- When the child later calls `exit()`, the kernel automatically sends
  `SIGCHLD` to the parent — this is **asynchronous** notification: the
  parent can be doing other work and still gets interrupted the moment
  its child dies.
- The handler reaps the child with `waitpid(..., WNOHANG)` to avoid
  leaving a zombie process.

---

## 7. How the Three COs Relate to the Overall Project

All three files revolve around the same central theme: **the operating
system's control over programs while they execute.**

- **CO-1** shows *why* the OS needs to protect a running executable —
  because processes are actively executing that file's code.
- **CO-2** shows *how* processes such as that running executable are
  actually created and destroyed in the first place (`fork` → `exec`
  → `wait`), which is exactly the mechanism that puts a program into
  the "currently running" state that CO-1 protects.
- **CO-3** shows *how processes coordinate and are notified* about
  each other's state changes (via pipes and `SIGCHLD`), which is the
  same kind of process-termination event that CO-2's `waitpid()`
  handles — but here observed asynchronously through a signal instead
  of a blocking call.

Together they form one coherent story about the life of a process:
**it is created, it runs (and is protected while running), it
communicates, and its termination is reported back to its parent.**

---

## 8. Compilation Commands

Make sure you are inside the project folder:

```bash
cd Denying-Writes-to-Executables
```

Compile everything at once using the provided Makefile:

```bash
make
```

This produces four executables:
```
CO1_Kernel_Protection
CO2_Process_Control
CO3_IPC_Signals
target_program
```

To clean up (remove compiled binaries):

```bash
make clean
```

If you prefer compiling manually without `make`:

```bash
gcc -Wall -Wextra -std=c99 -o CO1_Kernel_Protection CO1_Kernel_Protection.c
gcc -Wall -Wextra -std=c99 -o CO2_Process_Control CO2_Process_Control.c
gcc -Wall -Wextra -std=c99 -o CO3_IPC_Signals CO3_IPC_Signals.c
gcc -Wall -Wextra -std=c99 -o target_program target_program.c
```

---

## 9. Execution Commands

Run each demonstration separately:

```bash
./CO1_Kernel_Protection
./CO2_Process_Control
./CO3_IPC_Signals
```

> Note: `CO2_Process_Control` and `CO3_IPC_Signals` both internally run
> `./target_program`, so make sure `target_program` has been compiled
> and is present in the same folder (running `make` handles this
> automatically).

---

## 10. Expected Output

### CO1_Kernel_Protection
```
=====================================================
 CO1: Kernel Protection Against Writing to Executables
=====================================================

Step 1: Discovering the path of my own running executable...
   -> My executable path is: /home/user/.../CO1_Kernel_Protection
   -> My Process ID (PID) is: 12345

Step 2: Attempting to open my own executable for WRITING
        while it is currently running...

Attempting to open executable for writing...
WRITE DENIED
errno = 26
ETXTBSY: Text file busy
Linux kernel prevented the write.

EXPLANATION:
The kernel refused to let us open this executable
for writing because it is currently being executed
as a running process. ...

=====================================================
 CO1 demonstration complete.
=====================================================
```

### CO2_Process_Control
```
=====================================================
 CO2: Process Control (fork -> exec -> wait)
=====================================================

[Parent] My PID is 12401. I am about to create a child process.
[Parent] Child process created with PID = 12402
[Parent] Waiting for the child process to finish...

[Child ] Child process created successfully.
[Child ] My PID is 12402, my Parent's PID is 12401.
[Child ] Now calling execve() to run './target_program'...

[target_program] Hello! My PID is 12402
[target_program] I am now running...
[target_program] Work finished. Exiting normally now.

[Parent] Child process 12402 has terminated.
[Parent] Child exited normally with exit code 0.

=====================================================
 CO2 demonstration complete.
=====================================================
```

### CO3_IPC_Signals
```
=====================================================
 CO3: IPC (pipe) and Signals (SIGCHLD)
=====================================================

[Parent] Pipe created. Read end = 3, Write end = 4
[Parent] SIGCHLD handler installed.

[Parent] Child process created with PID = 12501
[Parent] Waiting to receive message from child via pipe...
[Child ] My PID is 12501. Sending READY message through the pipe...
[Parent] Received message from child: "READY"

[Parent] Now waiting for asynchronous SIGCHLD notification when the child terminates...
[Child ] Message sent. Now doing a little work and then exiting...
[Parent] (still waiting... doing other work)
[Child ] Terminating now (this will trigger SIGCHLD in the parent).
[Parent] >>> SIGCHLD received! Kernel notified us that the child has terminated. <<<

[Parent] Confirmed: child process has fully terminated and was reaped.

=====================================================
 CO3 demonstration complete.
=====================================================
```

*(Exact PID numbers and the interleaving of parent/child lines will
vary slightly each time you run the programs — this is normal and
expected behavior for concurrently running processes.)*

---

## 11. Software / Tools Required

- A Linux-based OS (Ubuntu 20.04+ recommended) or WSL on Windows
- `gcc` (GNU Compiler Collection) — C compiler
- `make` — build automation tool
- A terminal
- Basic familiarity with POSIX system calls (`fork`, `exec`, `wait`,
  `pipe`, `signal`, `open`)

No external libraries are required — everything uses the standard C
library and POSIX/Linux system headers only.

---

## Project Folder Structure

```
Denying-Writes-to-Executables/
│
├── CO1_Kernel_Protection.c   -> CO1: kernel protection / ETXTBSY
├── CO2_Process_Control.c     -> CO2: fork / execve / waitpid
├── CO3_IPC_Signals.c         -> CO3: pipe / SIGCHLD
├── target_program.c          -> helper program launched by CO2 & CO3
├── README.md                 -> this file
└── Makefile                  -> builds all four executables
```
