#!/bin/bash

OS=$1
TARGETARCH=$2
BUILDARCH=$3

apt-get update
apt-get install -y jq cmake build-essential

SCRIPTDIR=$(realpath $(dirname $0))
if [[ $TARGETARCH != $BUILDARCH ]]; then
    PLATFORMS_JSON="$SCRIPTDIR/platforms.json"

    COMPILEROS_JSONPATH=".$OS.name"
    COMPILEROS=$(cat $PLATFORMS_JSON | jq -r $COMPILEROS_JSONPATH)

    COMPILERARCH_JSONPATH=".$OS.architectures.$TARGETARCH"
    COMPILERARCH=$(cat $PLATFORMS_JSON | jq -r $COMPILERARCH_JSONPATH)

    COMPILERNAME=$COMPILERARCH-$COMPILEROS
    apt-get install -y gcc-$COMPILERNAME binutils-$COMPILERNAME

    dpkg --add-architecture $TARGETARCH
    apt-get update
fi

PACKAGES=$(cat $SCRIPTDIR/packages.json | jq -r ".build[]")
for PACKAGE_NAME in $PACKAGES; do
    PACKAGE_LIST="$PACKAGE_LIST $PACKAGE_NAME:$TARGETARCH"
done

apt-get install -y $PACKAGE_LIST
