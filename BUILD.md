# Build

## Prerequisites

The following packages need to be installed:

 * git
 * python
 * gcc (5.3 or higher)
 * g++
 * puppet
 * gtest (optional, see below)

*Note: casual will not build on a 32-bit system*

## Clone repo

If you're planning on trying some of the examples there might be a good idea to use `$HOME/git` as your repo-root, then
the examples correspond exactly to your setup.

Latest version is on `develop` branch (default)

```bash
git clone https://bitbucket.org/casualcore/casual.git
cd casual
```

## Install dependencies with puppet

```bash
sudo puppet apply thirdparty/setup/casual.pp
```

## Set up the environment

Use templatefile to setup environment

```bash
cp middleware/example/env/casual.env .
```

Edit `casual.env`, to set correct paths that correspond to your setup. It
should be pretty clear what you need to alter in the file.

If you already have `gtest` installed, update the environment file with the
paths to the library, otherwise the bundled version will be installed.

Then source the file:

```bash
. casual.env
```

## Compile
     
```bash
casual-make
```
     
If you want to compile as much as possible in parallel you can use:

```bash
casual-make compile && casual-make link
```

## Test

```bash
casual-make test
```

Tested on e.g. OS X, Ubuntu and CentOS so far. More unix flavors needed.

Please report any failed test cases to casual@laz.se.

## Install

Casual will be installed in `$CASUAL_HOME`:

```bash
casual-make install
```
