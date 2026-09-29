# =====================================================================
# Makefile for "Denying Writes to Executables" OS Project
# ---------------------------------------------------------------------
# Builds four separate executables, one per source file:
#     CO1_Kernel_Protection
#     CO2_Process_Control
#     CO3_IPC_Signals
#     target_program
#
# Usage:
#     make            -> builds all executables
#     make clean      -> removes all compiled binaries
# =====================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Names of the final executables (no .c extension)
TARGETS = CO1_Kernel_Protection CO2_Process_Control CO3_IPC_Signals target_program

all: $(TARGETS)

CO1_Kernel_Protection: CO1_Kernel_Protection.c
	$(CC) $(CFLAGS) -o $@ $<

CO2_Process_Control: CO2_Process_Control.c
	$(CC) $(CFLAGS) -o $@ $<

CO3_IPC_Signals: CO3_IPC_Signals.c
	$(CC) $(CFLAGS) -o $@ $<

target_program: target_program.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
