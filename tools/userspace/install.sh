#!/bin/bash -e
SCRIPT_DIR=`dirname $0`

if [ "$1" == "" ] || [[ "$2" == "" ]] || [[ "$3" == "" ]]; then
    echo "syntax: $0 <app.elf> <laritos.bin> <laritos.img>"
    exit 1
fi

echo Installing app $1 into $2 and generating img $3
cat $1 >> $2
dd if=/dev/zero of=$3 bs=1M count=64 status=none
dd if=$2 of=$3 conv=notrunc status=none
echo Done