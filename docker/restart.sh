#! /bin/bash

docker stop domainC
docker rm domainC

docker run --detach \
       -v $( pwd)/configC.json:/opt/casual/webapp/build/unbundled/src/config.json \
       -v $( pwd)/nginx.conf:/opt/casual/nginx/conf/nginx.conf \
       -v $( pwd)/domainC.yaml:/test/casual/configuration/domain.yaml \
       -h domainC \
       --name="domainC" \
       -p 8888:8080 \
       casual/middleware-centos

