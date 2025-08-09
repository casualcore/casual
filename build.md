
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
$ conan install conanfile.txt
...
```
If that fails use this command to build dependencies from source
```bash
$ conan install conanfile.txt --build=missing
...
```
**Note**: If you want to place your build binarys on another place then the default directory _build_ in your repo, you must add a exlicit path in conan install i.e.
```
$ conan install conanfile.txt --build=missing --output-folder /tmp/your_build_root
```
Then your binarys will end up under /tmp/your_build_root/build/Release

## build casual
     
```bash
$ cd $HOME/git/casual
$ cmake --preset conan-release # Configure cmake, made once
$ cmake --build . --preset conan-release # Build
```

## test casual

```bash
$ cmake --build . --preset conan-release --target test
```

## defined aliases i env
```bash
alias cmake-build='cmake --build . --preset conan-release'
alias cmake-make='cmake --preset conan-release'
alias cmake-test='cmake --build . --preset conan-release --target test'
```
