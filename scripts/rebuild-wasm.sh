#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
bname=`basename $0 .sh`

. $rootdir/scripts/setup-emscripten.sh

logfile=$rootdir/$bname-wasm.log
rm -f $logfile

CPU_COUNT=`getconf _NPROCESSORS_ONLN`

# run 'dpkg-reconfigure locales' enable 'en_US.UTF-8'
export LANG=en_US.UTF-8
export LC_MEASUREMENT=en_US.UTF-8

buildit() {
    echo rootdir $rootdir
    echo logfile $logfile
    echo CPU_COUNT $CPU_COUNT

    dist_dir="dist-wasm"
    build_dir="build-wasm"
    echo dist_dir $dist_dir
    echo build_dir $build_dir

    if [ -x /usr/bin/time ] ; then
        time_cmd="time"
        echo "time command available: ${time_cmd}"
    else 
        time_cmd=""
        echo "time command not available"
    fi

    cd $rootdir/$build_dir
    ${time_cmd} emmake make -j $CPU_COUNT install
    if [ $? -eq 0 ] ; then
        echo "REBUILD SUCCESS $bname wasm"
        cd $rootdir
        return 0
    else
        echo "REBUILD FAILURE $bname wasm"
        cd $rootdir
        return 1
    fi
}

buildit 2>&1 | tee $logfile

