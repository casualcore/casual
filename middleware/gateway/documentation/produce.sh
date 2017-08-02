#!/bin/bash

ROOT=$CASUAL_BUILD_HOME
GATEWAY=$ROOT/middleware/gateway

if [[ -z $CASUAL_HOME ]]
then
   export CASUAL_HOME=/tmp/casual/unittest
fi

export LD_LIBRARY_PATH=$GATEWAY/bin:$ROOT/middleware/common/bin:$ROOT/middleware/serviceframework/bin

echo "markdown"
$GATEWAY/documentation/protocol/bin/markdown-protocol > $GATEWAY/documentation/protocol/protocol.md

echo "binary"
$GATEWAY/documentation/protocol/bin/binary-protocol --base $GATEWAY/documentation/protocol/example/

