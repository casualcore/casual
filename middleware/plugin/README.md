## Build instructions

### The easy way
* Set environment with proper envfile for casual
* from reporoot enter: python thirdparty/setup/install_nginx.py

### The geeky way
* Set environment with proper envfile for casual
* Download current version of nginx from http://nginx.org/download/ and build it.

Example:

```sh
prompt$ wget http://nginx.org/download/nginx-1.10.2.tar.gz
prompt$ tar xvf nginx-1.10.2.tar.gz
prompt$ cd nginx-1.10.2/
prompt$ ./configure --with-cc-opt=-Wno-deprecated --add-module=$CASUAL_BUILD_HOME/plugin [perhaps also with --without-http_rewrite_module]
prompt$ make
prompt$ sudo make install
```
nginx are installed under /usr/local/nginx by default.

* Edit /usr/local/nginx/conf/nginx.conf

   At the moment the nginx worker process must execute with the same user as casual.

```
Add statement (or change) user.
Ex.
user  myuser staff;

Add location /casual to appropriate server entry.
Ex.
location /casual {
    casual_pass;
}
```     
