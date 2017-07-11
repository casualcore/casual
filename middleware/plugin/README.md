# Nginx plugin

Here is how you build the `casual` nginx plugin.

## The easy way

* Set environment with proper envfile for casual
* from reporoot enter: `python thirdparty/setup/install_nginx.py`

## The geeky way
* Set environment with proper envfile for casual
* Download current version of nginx from http://nginx.org/download/ and build it.

Example:

```bash
wget http://nginx.org/download/nginx-1.10.2.tar.gz
tar xvf nginx-1.10.2.tar.gz
cd nginx-1.10.2/
./configure --with-cc-opt=-Wno-deprecated --add-module=$CASUAL_BUILD_HOME/plugin 
# perhaps also with --without-http_rewrite_module
make
sudo make install
```

`nginx` is installed under `/usr/local/nginx` by default.

Edit `/usr/local/nginx/conf/nginx.conf`.

At the moment the `nginx` worker process must execute with the same user as casual.

Example:
```
user  myuser staff;

location /casual {
    casual_pass;
}
```     
