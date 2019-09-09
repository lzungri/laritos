#!/bin/bash -e
SCRIPT_DIR=`dirname $0`

source ${SCRIPT_DIR}/../../bin/include/config/auto.conf

DEBUG=
if [ "$1" == "-d" ]; then
    echo Launching qemu in debugging mode...
    DEBUG="-S -s"
fi

RAM_SIZE=$(( $CONFIG_SYS_RAM_SIZE / 1024 / 1024 ))

CPU=
if [ ! -z "$CONFIG_SYS_CPU_CORTEX_A15"  ] || [ ! -z "$CONFIG_SYS_CPU_GENERIC_ARMV7A" ]; then
    CPU=cortex-a15
elif [ ! -z "$CONFIG_SYS_CPU_CORTEX_A9" ]; then
    CPU=cortex-a9
fi

if [ -z "$CPU" ]; then
    echo "Couldn't find CPU name from auto.conf:"
    cat ${SCRIPT_DIR}/../../bin/include/config/auto.conf | awk '{print "\t" $0}'
    exit 1
fi

set -x
qemu-system-arm $DEBUG -M virt -smp 4 -m ${RAM_SIZE}M -cpu $CPU -nographic -drive if=pflash,file=${SCRIPT_DIR}/../../bin/laritos.img,format=raw -d guest_errors,cpu_reset,int,unimp -D /tmp/qemu.log
