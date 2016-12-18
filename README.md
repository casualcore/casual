# casual

*Disclaimer: First of all, this is the first time any of us has published an open source product. So, we don't really have any
experience how this stuff works*

## products

casual has a few 'products' that could be split into separate repos in the future, but for now
we keep them all in this repository.

### middleware
casual main purpose is [casual-middelware](/middleware/readme.md), which is an XATMI implementation

### make
[casual-make](/make/readme.md) is the 'build system' that is used to build casual

## Getting started

### build casual

Follow the [build instructions](build.md)

### Use casual-middleware

See [examples](/middleware/example/domain/readme.md)


### Status

#### Whats left to do?
* JCA implementation
* COBOL bindings
* some redesign of internal parts (for maintainability)


### field tests

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

