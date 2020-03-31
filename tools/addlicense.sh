#!/bin/bash -e

[ ! $# -eq 2 ] && echo "syntax: $0 <licensefile> <target>" && exit 1

[ ! -f $1 ] && echo "$1 not found" && exit 1

[ ! -d $2 ] && echo "$2 is not a valid directory or file" && exit 1

echo "Adding '$1' header to:"
for f in `find $2 -name '*.c'`; do
    if [ "`grep Copyright $f`" = "" ]; then
        echo "$f"
        cat $1 $f > $f.new && mv $f.new $f
    fi
done
