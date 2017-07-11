# casual

*Disclaimer: First of all, this is the first time any of us has published an open source product. So, we don't really have any
experience how this stuff works*

## Table of Contents

- [Products](#markdown-header-products)
  * [Make](#markdown-header-make)
  * [Middleware](#markdown-header-middleware)
  * [Thirdparty](#markdown-header-thirdparty)
    + [Database](#markdown-header-database)
    + [Setup](#markdown-header-setup)
  * [Webapp](#markdown-header-webapp)
- [Getting started](#markdown-header-getting-started)
  * [Build](#markdown-header-build)
  * [Usage](#markdown-header-usage)
- [Status](#markdown-header-status)
  * [Whats left to do?](#markdown-header-whats-left-to-do)
  * [Field tests](#markdown-header-field-tests)
- [Contribution guidelines](#markdown-header-contribution-guidelines)
- [Contact](#markdown-header-contact)
- [License](#markdown-header-license)
- [Summary](#markdown-header-summary)

## Products

Casual has a few 'products' that could be split into separate repos in the future, but for now
we keep them all in this repository.

### Make

[`casual-make`](./make/README.md) is the 'build system' that is used to build casual.

### Middleware

Casuals main purpose is `casual-middelware`, which is an XATMI implementation.

Read more in [`middleware/`](./middleware/README.md).

### Thirdparty

Contains third party libraries/products that casual has dependencies on and is easy enough
to bundle. That is, to make it easier for the developer to get started building casual.

#### Database

Just a thin C++ wrapper around sqlite API, to make it easier to use.

#### Setup

[`thirdparty/setup/`](./thirdparty/setup/README.md) contains `puppet` scripts to help setup environment with 
thirdparty dependencies.

### Webapp

Web application built with the power of web components. Read more in [`webapp/`](./webapp/README.md).

Requirements documented for the web application can be found in [`documentation/`](./documentation/requirements.web-gui.md).

## Getting started

### Build

Follow the [build instructions](./BUILD.md).

### Usage

See [examples](./middleware/example/domain/README.md) for `casual-middleware`.

We also provide information about how to run an example in docker, read more in [`docker/`](./docker/README.md).

## Status

### Whats left to do?

* JCA implementation
* COBOL bindings
* Some redesign of internal parts (for maintainability)

### Field tests

We've done some field tests:

* 1 XATMI domain (no inter-domain communications)
* 2 types of resources - IBM db2 and casual-queue
* Performance is good
* Scalability is really good, hence we feel confident that the basic design is good.

*We'll publish the result of the tests as soon as we can*.

## Contribution guidelines ###

* We have to get this whole project documented and organized before we define these guidelines.
* But if you made improvements, please keep the same look and feel of the code.

## Contact ###

* casual@laz.se

## License

Our intention is that everything in this repository is licensed under the [MIT licence](https://opensource.org/licenses/MIT),
with the exception of stuff under [`thirdparty/`](./thirdparty/README.md), which has their own licenses.

We (think we) show this in the [LICENSE](./LICENSE.md) file. If this is not enough legally, please enlighten us!

## Summary

Below is a list of all the included documentation.

<!-- summary below -->
* [./BUILD.md](./BUILD.md)
* [./LICENSE.md](./LICENSE.md)
* [./docker/README.md](./docker/README.md)
* [./documentation/requirements.web-gui.md](./documentation/requirements.web-gui.md)
* [./make/README.md](./make/README.md)
* [./middleware/README.md](./middleware/README.md)
* [./webapp/README.md](./webapp/README.md)
* [./middleware/buffer/README.md](./middleware/buffer/README.md)
* [./middleware/plugin/README.md](./middleware/plugin/README.md)
* [./thirdparty/setup/README.md](./thirdparty/setup/README.md)
* [./middleware/administration/documentation/api.md](./middleware/administration/documentation/api.md)
* [./middleware/buffer/documentation/field.md](./middleware/buffer/documentation/field.md)
* [./middleware/buffer/documentation/octet.md](./middleware/buffer/documentation/octet.md)
* [./middleware/buffer/documentation/order.md](./middleware/buffer/documentation/order.md)
* [./middleware/buffer/documentation/string.md](./middleware/buffer/documentation/string.md)
* [./middleware/common/documentation/log.md](./middleware/common/documentation/log.md)
* [./middleware/configuration/example/README.md](./middleware/configuration/example/README.md)
* [./middleware/domain/documentation/api.md](./middleware/domain/documentation/api.md)
* [./middleware/example/domain/README.md](./middleware/example/domain/README.md)
* [./middleware/example/ide/README.md](./middleware/example/ide/README.md)
* [./middleware/gateway/documentation/api.md](./middleware/gateway/documentation/api.md)
* [./middleware/gateway/documentation/protocol.md](./middleware/gateway/documentation/protocol.md)
* [./middleware/service/documentation/api.md](./middleware/service/documentation/api.md)
* [./middleware/transaction/documentation/api.md](./middleware/transaction/documentation/api.md)
* [./middleware/example/domain/multiple/medium/README.md](./middleware/example/domain/multiple/medium/README.md)
* [./middleware/example/domain/multiple/minimal/README.md](./middleware/example/domain/multiple/minimal/README.md)
* [./middleware/example/domain/single/minimal/README.md](./middleware/example/domain/single/minimal/README.md)
