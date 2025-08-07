#!/bin/bash

OS=$1
TARGETARCH=$2
BUILDARCH=$3

apt-get update
apt-get install -y jq cmake build-essential

if [[ $TARGETARCH != $BUILDARCH ]]; then
    SCRIPTDIR=$(realpath $(dirname $0))
    PLATFORMS_JSON="$SCRIPTDIR/platforms.json"

    COMPILEROS_JSONPATH=".$OS.Name"
    COMPILEROS=$(cat $PLATFORMS_JSON | jq -r $COMPILEROS_JSONPATH)

    COMPILERARCH_JSONPATH=".$OS.Architectures.$TARGETARCH"
    COMPILERARCH=$(cat $PLATFORMS_JSON | jq -r $COMPILERARCH_JSONPATH)

    COMPILERNAME=$COMPILERARCH-$COMPILEROS
    apt-get install -y gcc-$COMPILERNAME binutils-$COMPILERNAME

    dpkg --add-architecture $TARGETARCH
    apt-get update
fi

apt-get install -y libcurl4-openssl-dev:$TARGETARCH
