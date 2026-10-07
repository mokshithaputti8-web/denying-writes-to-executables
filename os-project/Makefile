# =====================================================================
# Makefile - Denying Writes to Executables
#   make        build everything
#   make run    build and run all programs one after another
#   make clean  remove binaries
# =====================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=gnu11
LDFLAGS = -pthread

TARGETS = CO1_Kernel_Protection CO2_Process_Control CO3_IPC_Signals \
          CO4_Virtual_Memory CO5_File_System CO6_Thread_Sync target_program

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: all
	@./run_all.sh

clean:
	rm -f $(TARGETS) demo_copy demo_copy.new

.PHONY: all run clean
