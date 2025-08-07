#!/bin/bash

apt-get update
apt-get install -y jq

SCRIPTDIR=$(realpath $(dirname $0))
PACKAGES=$(cat "$SCRIPTDIR/packages.json" | jq -r ".runtime[]")

for PACKAGE_NAME in $PACKAGES; do
    PACKAGE_LIST="$PACKAGE_LIST $PACKAGE_NAME"
done

apt-get install -y $PACKAGE_LIST
