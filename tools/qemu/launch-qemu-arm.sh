#!/bin/bash -e
SCRIPT_DIR=`dirname $0`

DEBUG=
if [ "$1" == "-d" ]; then
    echo Launching qemu in debugging mode...
    DEBUG="-S -s"
fi

qemu-system-arm $DEBUG -M virt -smp 4 -m 1G -cpu cortex-a15 -nographic -drive if=pflash,file=${SCRIPT_DIR}/../../bin/laritos.img,format=raw
