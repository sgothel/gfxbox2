#!/bin/bash

fps=10
#fps=30
#fps=60

bname=$1

if [ -z "${bname}" ] ; then
    echo "Usage $0 <basename of bmp-files> - basename.mp4 will be created"
    exit 1
fi

ofname="${bname}.mp4"
rm -f ${ofname}

#
# https://stackoverflow.com/questions/24961127/how-to-create-a-video-from-images-with-ffmpeg
# http://hamelot.io/visualization/using-ffmpeg-to-convert-a-set-of-images-into-a-video/
#

ffmpeg -f image2 -pattern_type glob -r ${fps} -i ${bname}'*.bmp' -shortest -c:v libx264 -vf "fps=${fps},format=yuv420p" ${ofname}
