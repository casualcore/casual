# Setup 

This directory contain some helper scripts to setup environment with thirdparty dependencies via `puppet`.

## Build dependencies

To setup basic environment use `puppet`:

```bash
puppet apply casual.pp
```

Just in case you have to manually build and install `yaml-cpp`:

```bash
python install_yaml.py	
```
    
## Nginx plugin

To build and setup `nginx` plugin make sure that casual is built and environment set and then:

```bash
python install_nginx.py
```
