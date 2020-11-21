
## build casual



### Prerequisites
The following packages need to be installed:

 * git
 * python
 * gcc (version 6)
 * g++ (version 6)
 * cmake
 * puppet
 * gtest (included, see below)

*Note: casual will not build on a 32-bit system*


### clone repo

If you're planning on trying some of the examples it might be a good idea to use `$HOME/git` as your repo-root, then 
the examples correspond exactly to your setup.

Latest version is on `develop` branch (default)

```bash
>$ git clone https://bitbucket.org/casualcore/casual.git
>$ cd casual
```

### CentOS Setup

Enable EPEL, Software Collections and install additional required dependencies:

```bash
>$ sudo yum install epel-release centos-release-scl
>$ sudo yum install libuuid-devel pugixml-devel sqlite-devel python libcurl-devel devtoolset-6
>$ scl enable devtoolset-6 bash
```

Compile the yaml lib:

```bash
>$ cd thirdparty/setup
>$ CMAKE_CXX_COMPILER=g++ python install_yaml.py
```

Use the templatefile to setup environment:

```bash
>$ cp middleware/example/env/casual.env .
```

Edit the yaml include and library paths:

```bash
>$ YAML_INCLUDE_PATH=/usr/local/include/yaml-cpp/
>$ YAML_LIBRARY_PATH=/usr/local/lib/
```

Then follow the instructions from Build Casual

### Install dependencies with puppet

```bash
>$ sudo puppet apply thirdparty/setup/casual.pp
```

### Set up the environment 

Use the templatefile to setup environment

```bash
>$ cp middleware/example/env/casual.env .
```

Edit `casual.env` to set the correct paths that correspond to your setup. It
should be pretty clear what you need to alter in the file.

Then source the file:

```bash
>$ source casual.env
```

### Build casual
     
```bash
>$ casual-make
```

If you want to compile as much as possible in parallel you can use:

```bash
>$ casual-make compile && casual-make link
```

### Test casual

```bash
>$ casual-make test
```

Tested on e.g. OS X and Ubuntu so far. More unix flavors needed

Please report failed test cases (contact info below)   
