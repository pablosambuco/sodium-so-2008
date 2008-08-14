#!/bin/sh

if test $# -ne 3 
then
	echo "Tool to extract symbol addresses from object files."
	exit 1
fi

if test ! -f $1
then
	echo "object file doesn't exist"
	exit 1
fi

echo 0x`nm -f posix $1 | grep "$3 $2" | cut -d' ' -f3`

exit 1
