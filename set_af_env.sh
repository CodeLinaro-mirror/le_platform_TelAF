#setup toolchain - this can be optimized later with findtoolchain script

if [ "$1" == "sa415m" ]; then
    source /opt/qct/sa415m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
elif [ "$1" == "sa515m" ]; then
    source /opt/qct/sa515m/environment-setup-armv7at2hf-neon-oe-linux-gnueabi
else
    echo " Missing target parameter!"
    echo " e.g. $0 sa415m"
    exit
fi

umask 002


#build the target

function build-sa415m-af(){
    make sa415m
}

function build-clean-af(){
    make clean
}

function build-distclean-af(){
    make distclean
}

function build-sa515m-af(){
    make sa515m
}


