
## build casual



### Prerequisites
The following packages need to be installed:

 * git
 * python
 * gcc (5.3 or higher)
 * g++
 * puppet
 * gtest (included, see below)

*Note: casual will not build on a 32-bit system*


### clone repo

if you're planning on trying some of the examples there might be a good idea to use `$HOME/git` as your repo-root, then 
the examples correspond exactly to your setup.

Latest version is on `develop` branch (default)

    host$ git clone https://bitbucket.org/casualcore/casual.git
    host$ cd casual


### Install dependencies with puppet

    host$ sudo puppet apply thirdparty/setup/casual.pp


### Set up the environment

Use templatefile to setup environment

    host$ cp middleware/example/env/casual.env .


Edit `casual.env`, to set correct paths that correspond to your setup. It
should be pretty clear what you need to alter in the file.

Then source the file

    host$ source casual.env

### Build casual
     
    host$ casual-make
     
If you want to compile as much as possible in parallel you can use:

     host$ casual-make compile && casual-make link

### Test casual

     host$ casual-make test


Tested on e.g. OS X and Ubuntu so far. More unix flavors needed

Please report failed test cases (contact info below)   
