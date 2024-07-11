#!/bin/sh

if [ -z "$EMSDK" ] ; then
    export EM_CONFIG=$HOME/.emscripten
fi

#
# emcc --generate-config
# emcc --clear-cache
# emcc --clear-ports
#

