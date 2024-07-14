#! /bin/sh

corein=$1

if [ -z "$corein" -o ! -e "$corein" ] ; then
    echo "Usage $0 <core-file>"
    exit 1
fi

sudo chown $USER $corein
rm -f core
zstd -d -o core $corein
rm -f $corein
