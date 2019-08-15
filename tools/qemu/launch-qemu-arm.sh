#!/bin/bash -e
SCRIPT_DIR=`dirname $0`

echo Launching qemu in debugging mode...
qemu-system-arm -M virt -smp 4 -m 1G -cpu cortex-a15 -nographic -S -s -drive if=pflash,file=${SCRIPT_DIR}/../../bin/laritos.img,format=raw
