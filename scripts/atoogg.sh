#!/bin/sh

bname=`basename $1 .mp3`

ffmpeg -i $1 -c:a libvorbis -q:a 1 $bname.ogg
