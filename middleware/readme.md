# casual-middleware

The main product in the casual suite.

## what is casual-middleware?

* In short: 
    * SOA platform and Distributed transaction manager
* Implementation of (hence conformant to):
    * [XATMI specification](http://pubs.opengroup.org/onlinepubs/9694999399/toc.pdf)
    * [XA specification](http://pubs.opengroup.org/onlinepubs/009680699/toc.pdf)
    * [TX specification](http://pubs.opengroup.org/onlinepubs/9694999599/toc.pdf)


## objectives
* As little configuration as possible, since we believe that enterprises are in desperate need of reduced complexity.
* Performance should be good enough, hence make casual a viable choice when migrating from other XATMI implementations.
* Keep the cold cold, and the hot hot. To enable over provisioning, hence let the OS take care of scheduling.
* Productive. That is, the opposite of many of todays "application servers"


## Why casual?
* The semantics are simple and easy to reason about.
* Services (servers) can be distributed to fit any deployment needs.
* Asynchronous pattern gives massive concurrency without the hassle of threads
* Using single threaded servers gives linear scalability and execution encapsulation 
* Services can be exposed as REST services (JSONP, SOAP…), non intrusive out of the box\*
* Built on specifications that is proven to work for large scale applications with high performance demand

\*) If the service is implemented via the C++ abstraction layer.



## structure

There are a few main modules 


### domain

Responsible for a casual-domain. Boots all the other managers and user provided servers and such.

[documentation](./domain/readme.md)


### transaction

Responsible for distrubuted transactions.

[documentation](./transaction/documentation/api.md)


### service

Responsible for service lookup and management.

[documentation](./service/documentation/api.md)


### gateway

Responsible for communications with other domains

[documentation](./gateway/documentation/api.md)


### queue

A queue implementation.

[documentation](./queue/documentation/api.md)

### common

Functionality that is common and in some way shared with other modules. 

[documentation](./common/documentation/log.md)

### serviceframework

Service abstraction layer, to hide transportation details.

[documentation](./serviceframework/readme.md)



