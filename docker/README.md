# Docker Demo

## Overview

This is an example of how to run casual in two docker containers.

## Requirements

* [`docker`](https://docs.docker.com/engine/installation/)
* [`docker-compose`](https://docs.docker.com/compose/install/)

## Configuration

The configuration files used for `casual-middleware` are located in `${CASUAL_BUILD_HOME}/docker/config/domain{A,B}-config.yaml`
and is based on [`${CASUAL_BUILD_HOME}/middleware/example/domain/multiple/minimal/`](./../middleware/example/domain/multiple/minimal/readme.md).

## Start

```bash
$ cd ${CASUAL_BUILD_HOME}/docker
$ docker-compose up
```

## Test

First you need to know the IP addresses for the `domainA` and `domainB` containers, this can be done with `docker inspect`.
Example:

```bash
$ docker inspect -f '{{ .NetworkSettings.Networks.docker_isolated_nw.IPAddress }}' \
    $(docker ps | awk '/docker_domain[A-B]/ {print $1}')
172.22.0.3
172.22.0.2
```

Call `casual/example/echo` on either container (the actual service is running on `domainA`):

```bash
$ curl -d "Hello world" http://172.22.0.2:8080/casual?service=casual/example/echo
$ curl -d "Hello world" http://172.22.0.3:8080/casual?service=casual/example/echo
```

Attach to one of the domains, and play around with `casual`. Example:

```bash
$ docker exec -it docker_domainA_1 bash
[casual@domainA casual]$ casual gateway --list-connections
name     id                                bound  pid  queue   type  runlevel  address
-------  --------------------------------  -----  ---  ------  ----  --------  -----------------------------------------
domainB  9ace02251b6647e4838fcba25b12e47a  in      18  262151  tcp   online    docker_domainB_1.docker_isolated_nw:39094
[casual@domainA casual]$ casual service --list-services
name                         category  mode  timeout  LI  LC  LAT     P  PAT     RI  RC  last
---------------------------  --------  ----  -------  --  --  ------  -  ------  --  --  -----------------------
casual/example/conversation  example   join   0.0000   1   0  0.0000  0  0.0000   0   0  0000-00-00T00:00:00.000
casual/example/echo          example   join   0.0000   1   2  0.0009  0  0.0000   0   0  2017-07-20T07:19:36.942
casual/example/rollback      example   join   0.0000   1   0  0.0000  0  0.0000   0   0  0000-00-00T00:00:00.000
casual/example/sink          example   join   0.0000   1   0  0.0000  0  0.0000   0   0  0000-00-00T00:00:00.000
casual/example/uppercase     example   join   0.0000   1   0  0.0000  0  0.0000   0   0  0000-00-00T00:00:00.000

```

## Stop

When done with the demo, clean up afterwards with:

```
$ docker-compose down
```
