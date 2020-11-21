
# casaul middleware domain

## domain-manager:
* Responsible to handle all configuration
* Handle the lifecycle of every process in the domain (not counting grand-children)

### TODO
* Store configuration persistent - not sure about the semantics though



## domain-delay

Delays a message for the requested time before it is passed to the destination.

Can (and is) be used as an "active sleep". That is, some module waits for something to be ready and instead of doing 
a sleep for some time, it sends a message to *delay* and keep running the message loop, hence act as normal and no
special corner cases is needed. When the delay has passaed, the module is invoked with the message and dispatched to
a suitable handle (exactly as any other invocation).




