
# build casual

## prerequisites

The following packages need to be installed:

 * git
 * gcc (version >= 14.2)
 * g++ (version >= 14.2)
 * cmake
 * conan

*Note: casual will not build on a 32-bit system*

**system environment variables**

variable        | used for
----------------|------------------------------
`TMPDIR`        | well, temporary files...


## clone

if you're planning on trying some of the examples there might be a good idea to use `$HOME/git` as your repo-root, then 
the examples correspond exactly to your setup.

```bash
$ git clone https://github.com/casualcore/casual.git
```


## set up the environment

Enter the casual repo.

```bash
$ cd $HOME/git/casual
```

It should be enough to just source the example environment set up file.

```bash
$ source middleware/example/env/casual.env
```

### custom setup 

If you got another setup or there are some platform specific problem, you need
to edit the _casual.env_ file to suit your platform setup.

```bash
$ cp middleware/example/env/casual.env .
$ vim casual.env # edit to suit your needs
$ source casual.env
```

## Install dependencies with conan

```bash
$ cd $HOME/git/casual
$ conan profile detect # if never used conan before
$ conan install . --output-folder=../casual-conan-dependencies --deployer=direct_deploy
...
```
If that fails use this command to build dependencies from source
```bash
$ conan install . --build=missing --output-folder=../casual-conan-dependencies --deployer=direct_deploy
...
```

## build casual
     
```bash
$ cd $HOME/git/casual
$ casual-make compile && casual-make link
```

## test casual

```bash
$ casual-make test
```
