#!/bin/bash
# Runs every demo one after another. Build first with: make
cd "$(dirname "$0")" || exit 1
for prog in CO1_Kernel_Protection CO2_Process_Control CO3_IPC_Signals \
            CO4_Virtual_Memory CO5_File_System CO6_Thread_Sync; do
    if [ ! -x "./$prog" ]; then
        echo "Binary ./$prog not found - run 'make' first."; exit 1
    fi
    "./$prog"
    echo
done
