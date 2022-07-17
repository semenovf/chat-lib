#!/bin/bash

CWD=`pwd`

CEREAL_RELEASE=v1.3.2

if [ -e .git ] ; then

    git checkout master && git pull origin master \
        && git submodule update --init --recursive \
        && git submodule update --init --recursive --remote -- 3rdparty/portable-target \
        && git submodule update --init --recursive --remote -- 3rdparty/pfs/common \
        && git submodule update --init --recursive --remote -- 3rdparty/pfs/debby \
        && git submodule update --init --recursive --remote -- 3rdparty/pfs/jeyson \
        && git submodule update --init --recursive --remote -- 3rdparty/pfs/netty \
        && cd 3rdparty/cereal && git checkout $CEREAL_RELEASE \
        && cd $CWD

fi

