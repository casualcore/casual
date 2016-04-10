This directory contain some helper scripts to setup environment with thirdparty dependencies.

To setup basic environment use puppet:
    puppet apply casual.pp
    
To build and setup nginx plugin make sure that casual is build and environment set and then:
    python install_nginx.py
    
Just in case you have to manually build and install yaml-cpp:
    python install_yaml.py	