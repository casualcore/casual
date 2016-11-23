# casual

*Disclaimer: First of all, this is the first time any of us has published an open source product. So, we don't really have any
experience how this stuff works*

## products

casual has a few 'products' that could be split into separate repos in the future, but for now
we keep them all in this repository.

### middleware
casual main purpose is [casual-middelware](/middleware/readme.md), which is an XATMI implementation

### make
[casual-make](/tools/casual/make/readme.md) is a 'build system' that is easy to use.

Users declare their intent (in pure python) and casual-make take care of the rest.

Easy to implement DSL stuff to fit most needs.

Can of course be used stand alone without the rest of casual.

### Getting started ###

#### Prerequisites
The following packages need to be installed:

 * git
 * python
 * gcc (4.8.3 or higher)
 * g++
 * puppet
 * gtest (included, see below)

*Note: casual will not build on a 32-bit system*

#### Set up the environment
Latest version is on *develop* branch (default)

Use templatefile to setup environment

    git clone https://bitbucket.org/casualcore/casual.git
    cd casual
    cp middleware/example/env/casual.env .

Edit file, set correct paths and source file

    source casual.env

#### Install dependencies with puppet
    sudo puppet apply thirdparty/setup/casual.pp

#### Make included thirdparty for unit test
     cd $CASUAL_REPO_ROOT/casual/thirdparty/unittest/gtest
     casual-make

#### Build casual
     cd $CASUAL_BUILD_HOME
     casual-make compile && casual-make install && casual-make link

#### Test casual

     casual-make test

Tested on e.g. OS X and Ubuntu so far. More unix flavors needed

Please report failed test cases (contact info below)    

### Use casual-middleware
TODO: user/operation documentation to come.

Meanwhile there are some configuration examples that could shed some light under: middleware/example


### Status

#### Whats left to do?
* JCA implementation
* COBOL bindings
* some redesign of internal parts (for maintainability)

We've done some field tests

* 1 XATMI domain (no inter-domain communications)
* 2 types of resources - IBM db2 and casual-queue
* Performance is good
* Scalability is really good, hence we feel confident that the basic design is good.

*We'll publish the result of the tests as soon as we can*

### Contribution guidelines ###

* We have to get this whole project documented and organized before we define these guidelines.
* But if you made improvements, please keep the same look and feel of the code.

### Contact ###

* casual@laz.se


### License
Our intention is that everything in this repository is licensed under the [MIT licence](https://opensource.org/licenses/MIT),
with the exception of stuff under [thirdparty](/thirdparty/readme.md), which has their own licenses.

We (think we) show this by the file [licence.md](/license.md). If this is not enough legally, please enlighten us!