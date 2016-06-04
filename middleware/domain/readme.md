
# casaul middleware domain

Not implemented yet...

## Proposal:
* responsible to handle all configuration
* store configuration persistent - not sure about the samantics though
* handle the lifecycle of every process in the domain (not counting grand-children)

Then we can change broker to handle service-lookup, only. The responsibilities becomes more clear and intuitive.


## domain-delay

Delays a message for reqeusted time before it is passed to the destination

Can (and is) be used as an "active sleep". That is, some module wait's for something to be ready and instead of doing 
a sleep for some time, it sends a message to *delay* and keep running the message loop, hence act as normal and no
special corner cases is needed. When the delay has passaed, the module is invoked with the message and dispatched to
a suitable handle (exactly as any other invocation)




