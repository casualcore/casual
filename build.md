
# build casual

## prerequisites

The following packages need to be installed:

 * git
 * python
 * gcc (version 8)
 * g++ (version 8)
 * puppet

*Note: casual will not build on a 32-bit system*

**system environment variables**

variable        | used for
----------------|------------------------------
`TMPDIR`        | well, temporary files...


## clone

if you're planning on trying some of the examples there might be a good idea to use `$HOME/git` as your repo-root, then 
the examples correspond exactly to your setup.

```bash
host$ git clone https://bitbucket.org/casualcore/casual.git
host$ git clone https://bitbucket.org/casualcore/casual-thirdparty.git
```



## preperation

platform specific preperations


### CentOS

Enable EPEL, Software Collections and install stuff

```bash
sudo yum install epel-release centos-release-scl
sudo yum install libuuid-devel sqlite-devel python libcurl-devel devtoolset-8
scl enable devtoolset-8 bash
```

## set up the environment

Enter the casual repo.

```bash
host$ cd $HOME/git/casual
```

It should be enough to just source the example environment set up file.
(if the casual and casual-thirdparty repo's are next to eachother)

```bash
host$ source middleware/example/env/casual.env
```

### custom setup 

If you got another setup or there are some platform specific problem, you need
to edit the _casual.env_ file to suit your platform setup.

```bash
host$ cp middleware/example/env/casual.env .
host$ vim casual.env # edit to suit your needs
host$ source casual.env
```


## build casual
     
```bash
host$ casual-make
```
     
If you want to compile as much as possible in parallel you can use:

```bash
host$ casual-make compile && casual-make link
```

## test casual

```bash
host$ casual-make test
```

## feedback

If this _how-to_ is not to your liking, or does not describe your platform
please provide a pull-request to fix it.
