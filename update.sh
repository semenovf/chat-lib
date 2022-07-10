#!/bin/bash

CWD=`pwd`

CEREAL_RELEASE=v1.3.2

if [ -e .git ] ; then

    git pull \
        && git submodule update --init \
        && git submodule update --remote \
        && cd 3rdparty/portable-target && git checkout master && git pull \
        && cd $CWD \
        && cd 3rdparty/pfs/common && ./update.sh \
        && cd $CWD \
        && cd 3rdparty/pfs/debby && ./update.sh \
        && cd $CWD \
        && cd 3rdparty/pfs/jeyson && ./update.sh \
        && cd $CWD \
        && cd 3rdparty/pfs/netty && ./update.sh \
        && cd $CWD \
        && cd 3rdparty/cereal && git checkout $CEREAL_RELEASE \
        && cd $CWD

fi

