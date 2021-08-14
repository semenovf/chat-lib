#!/bin/sh

BASE_URL=https://raw.githubusercontent.com/semenovf/scripts/master

SCRIPTS="build.sh \
    build-windows.cmd"

CONFIGS="CMakeLists.txt \
    .gitignore \
    .travis.yml \
    appveyor.yml \
    codecov.yml"

LAST_STATUS=0
LAST_COMAMND=

curl_copy()
{
    FILENAME=$1
    SOURCE=$2
    TARGET=$3

    LAST_COMMAND="curl -s $SOURCE -o $TARGET"
    echo -n "Copying $FILENAME ... "

    $LAST_COMMAND > /dev/null
    LAST_STATUS=$?

    if [ $LAST_STATUS -eq 0 ] ; then
        echo "OK"
    else
        echo "ERROR: failure on command"
        echo "\t$LAST_COMMAND"
    fi
}

for f in $SCRIPTS ; do
    curl_copy $f $BASE_URL/scripts/${f} $f
    [ $LAST_STATUS -ne 0 ] && exit 1
done

for f in $CONFIGS ; do
    curl_copy $f $BASE_URL/configs/${f}  $f
    [ $LAST_STATUS -ne 0 ] && exit 1
done

echo "\nNow it is time to edit CMakeLists.txt to complete project basic initialization"

if [ -z "${EDITOR}" ] ; then
    if command -v mcedit > /dev/null ; then
        EDITOR=mcedit
    elif command -v nano > /dev/null ; then
        EDITOR=nano
    elif command -v vim > /dev/null ; then
        EDITOR=vim
    elif command -v vi > /dev/null ; then
        EDITOR=vi
    else
        echo "ERROR: No any known editor found"
    fi
fi

if [ -n "${EDITOR}" ] ; then
    $EDITOR CMakeLists.txt
fi
