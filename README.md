# casual

*Disclaimer: First of all, this is the first time any of us has published an open source product. So, we don't really have any
experience how this stuff works*

## products

casual has a few 'products' that could be split into separated repos in the future, but for now
we keep them all in this repository.

### middleware
casual main purpose is [casual-middelware](/middleware/readme.md), which is an XATMI implementation

### make
[casual-make](/tools/casual/make/readme.md) is a 'build system' that is easy to use.

Users declare their intent (in pure python) and casual-make take care of the rest.

Easy to implement DSL stuff to fit most needs.

Can of course be used stand alone without the rest of casual.

### How do I get set up? ###

#### Prerequisites
The following needs to be installed using rpm or yum packages:

 * git
 * python
 * gcc-c++
 * puppet

#### Set up the environment
Use templatefile to setup environment

    cp middleware/example/env/casual.env casual.env

Edit file, set correct paths and source file

    source casual.env

#### Install dependencies with puppet
    sudo puppet apply thirdparty/setup/casual.pp

#### Build casual
     cd $CASUAL_BUILD_HOME
     casual-make compile && casual-make install && casual-make link

#### Test casual

casual-make test


*tested om OS X and Ubuntu so far. More unix flavors needed* 
     

### use casual-middleware
TODO: this documentation should be separated from this repo? At least conceptually

### Status

#### Whats left to do?
* gateway
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

### Who do I talk to? ###

* Fredrik Eriksson (laz@laz.se)


### license
Our intention is that everything in this repository is licensed under the [MIT licence](https://opensource.org/licenses/MIT),
with the exception of stuff under [thirdparty](/thirdparty/readme.md), which has their own licenses.

We (think we) show this by the file [licence.md](/license.md). If this is not enough legally, please enlighten us!