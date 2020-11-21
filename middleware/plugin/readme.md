# Build instructions

## Prerequisite

- Set environment for casual. i.e CASUAL_BUILD_HOME and CASUAL_HOME
- Build casual

## Automatic build and install (preferred)

```bash
>$ python ${CASUAL_BUILD_HOME}/thirdparty/setup/install_nginx.py
```


OR

## Manual build and install

Download current version of nginx from http://nginx.org/download/ and build it.

Example:

```bash
>$ wget http://nginx.org/download/nginx-1.13.5.tar.gz
>$ tar xvf nginx-1.13.5.tar.gz
>$ cd nginx-1.13.5
>$ ./configure --with-cc-opt=-Wno-deprecated --without-http_rewrite_module --add-module=$CASUAL_BUILD_HOME/middleware/plugin
>$ make
>$ cp objs/nginx ${CASUAL_HOME}/nginx/sbin/nginx && cp ${CASUAL_BUILD_HOME}/thirdparty/nginx/nginx.conf ${CASUAL_HOME}/nginx/conf/nginx.conf
```
        

