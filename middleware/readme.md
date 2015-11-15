# casual-middleware

The main product in the casual suite.

## What is casual-middleware?

* In short: A better version of Tuxedo
    * SOA platform and Distributed transaction manager
* Implementation of (hence conformant to):
    * [XATMI specification](http://pubs.opengroup.org/onlinepubs/9694999399/toc.pdf)
    * [XA specification](http://pubs.opengroup.org/onlinepubs/009680699/toc.pdf)
    * [TX specification](http://pubs.opengroup.org/onlinepubs/9694999599/toc.pdf)


## Objectives
* Be on par with Tuxedo regarding stability.
* As little configuration as possible, since we believe that enterprises are in desperate need of reduced complexity.
* Performance should be good enough, hence make casual a viable choice when migrating from other XATMI implementations.
* Keep the cold cold, and the hot hot. To enable over provisioning, hence let the OS take care of scheduling.
* Productive. That is, the opposite of many of todays "application servers"


## Why casual?
* The semantics are simple and easy to reason about.
* Services (servers) can be distributed to fit any deployment needs.
* Asynchronous pattern gives massive concurrency without the hassle of threads
* Using single threaded servers gives linear scalability and execution encapsulation *(ie Chrome uses processes instead of threads to achieve higher security and stability)* 
* Services can be exposed as REST services (JSONP, SOAP…), non intrusive out of the box\*

\*) If the service is implemented via the C++ abstraction layer.



