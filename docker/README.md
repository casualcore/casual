### Overview
This is an example of how to run casual in two docker containers.

### Requirements
* You need a docker version that handles networks
* You need to create a bridge network called isolated_nw. (Of course you can call it something else but then you need to adjust the start accordingly

### How to start

    docker run --detach \
       -v $( pwd)/configA.json:/opt/casual/webapp/build/unbundled/src/config.json \
       -v $( pwd)/nginx.conf:/opt/casual/nginx/conf/nginx.conf \
       -v $( pwd)/domainA.yaml:/test/casual/configuration/domain.yaml \
       -h domainA \
       --name="domainA" \
       --net isolated_nw \
       casual/middleware

    docker run --detach \
       -v $( pwd)/configB.json:/opt/casual/webapp/build/unbundled/src/config.json \
       -v $( pwd)/nginx.conf:/opt/casual/nginx/conf/nginx.conf \
       -v $( pwd)/domainB.yaml:/test/casual/configuration/domain.yaml \
       -h domainB \
       --name="domainB" \
       --net isolated_nw \
       casual/middleware

### How to test
Check with ***docker inspect domainA*** and ***docker inspect domainB*** after the ipAddress.

And then run commands ruffly like this (regarding ip)

    curl -d "{}" http://172.18.0.2:8080/casual?service=.casual.broker.state    
    curl -d "Hello world" http://172.18.0.2:8080/casual?service=casual.example.echo
    curl -d "Hello world" http://172.18.0.3:8080/casual?service=casual.example.echo

