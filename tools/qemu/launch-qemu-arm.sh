#!/bin/bash -e
SCRIPT_DIR=`dirname $0`

source ${SCRIPT_DIR}/../../bin/include/config/auto.conf

DEBUG=
if [ "$1" == "-d" ] || [[ "$2" == "-d" ]]; then
    echo "Launching guest in debugging mode (listening on :1234)..."
    DEBUG="-S -s"
fi

DEBUG_QEMU=
if [ "$1" == "-D" ] || [[ "$2" == "-D" ]]; then
    echo "Launching qemu in debugging mode (listening on :55555)..."
    DEBUG_QEMU="gdbserver :55555"
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


echo
qemu-system-arm --version | grep version
echo ---------------------------------------------------------------------------
echo

TRACE_FILE=/tmp/qemu_trace
rm -f $TRACE_FILE

# Note: Configure qemu source tree with
#   `configure --target-list=arm-softmmu,arm-linux-user --enable-debug --enable-trace-backends=simple`
set -x
$DEBUG_QEMU qemu-system-arm --trace events=${SCRIPT_DIR}/trace_events,file=$TRACE_FILE $DEBUG -M virt -smp 4 -m ${RAM_SIZE}M -cpu $CPU -nographic -drive if=pflash,file=${SCRIPT_DIR}/../../bin/laritos.img,format=raw -d guest_errors,cpu_reset,int,unimp -D /tmp/qemu.log
set +x

echo ---------------------------------------------------------------------------


QEMU_DIR=`dirname $(which qemu-system-arm)`

echo
echo Trace report
$QEMU_DIR/../../../../scripts/simpletrace.py $QEMU_DIR/../trace-events-all $TRACE_FILE
