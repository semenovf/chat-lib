#!/bin/bash

LOG_FILE='download.log'
GIT_DOWNLOADER="git clone"

# SSH
SSH_SOURCES="git@github.com:semenovf/portable-target.git -b master portable-target
git@github.com:semenovf/common-lib.git -b master common
git@github.com:semenovf/debby-lib.git -b master debby
git@github.com:semenovf/ionik-lib.git -b master ionik
git@github.com:semenovf/jeyson-lib.git -b master jeyson
git@github.com:semenovf/lorem-lib.git -b master lorem
git@github.com:semenovf/mime-lib.git -b master mime
git@github.com:semenovf/scripts.git -b master scripts"

# HTTPS
HTTPS_SOURCES="https://github.com/semenovf/portable-target.git -b master portable-target
https://github.com/semenovf/common-lib.git -b master common
https://github.com/semenovf/debby-lib.git -b master debby
https://github.com/semenovf/ionik-lib.git -b master ionik
https://github.com/semenovf/jeyson-lib.git -b master jeyson
https://github.com/semenovf/lorem-lib.git -b master lorem
https://github.com/semenovf/mime-lib.git -b master mime
https://github.com/semenovf/scripts.git -b master scripts"

DEFAULT_SOURCES=${SSH_SOURCES}
DEFAULT_DOWNLOADER=${GIT_DOWNLOADER}

IFS=$'\n'

echo `date` > ${LOG_FILE}

for src in ${DEFAULT_SOURCES} ; do
    eval "${DEFAULT_DOWNLOADER} $src" >> ${LOG_FILE} 2>&1

    if [ $? -eq 0 ] ; then
        echo "Cloning $src: SUCCESS"
    else
        echo "Cloning $src: FAILURE"
    fi
done
