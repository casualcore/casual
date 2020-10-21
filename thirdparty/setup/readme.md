# setup

This directory contain some helper scripts to setup environment with thirdparty dependencies.

## basic

To setup basic environment use puppet:

```shell
host$ puppet apply casual.pp
```
    
## nginx

To build and setup nginx plugin make sure that casual is built and environments are set and then:

```shell
host$ python install_nginx.py
```
