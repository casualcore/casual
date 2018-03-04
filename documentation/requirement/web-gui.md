

# Requirements for casual middleware UI

This document tries to define the requirements for what is useful and paramount
for a user of casual middleware

## What we think users wants

There are several *views* that the user wants to se. Some of these can maybe 
be combined.


### groups

View and manage group information

* name
* note
* resources (read only?)
    * xa key, openinfo and such

*expand to see which executables is member of this group?*

**source: domain-manager**

### executables

We might want to split **servers** and **executables** into two different views.

View and manage exacutable information

List of executables with:

* alias (this is the external handle of the 'executable')
* note
* path
* instances (read only)
    * mange numbrer of instances (scale in, scale out)
* memberships
    * "expand" to group view?
    
*don't think we'll expose the internal id of the executable, user should only refer to 'alias'. The internal id shold be used to correlate instanses though* 
    
**source: domain-manager**
    
### instances

List of instances with:

* pid
* ipc-queue
* executable
    * "expand" to executable view?

**source: domain-manager**

### services

View and manage exacutable information

List of services with:

* name
* type
* transaction-semantic
* instances
    * "expand" to instance view?
    * if an instance is a outbound-connection (gateway) show info about this.

    
Should be easy to manage instanses from this view, since services should be what's interesting 

**source: broker**
 
 
### pending services

view information about pending requests

List of pending services with:

* name
* number of requests that has been pending
* total time that others has waited for this server to be callable
* calculated avarage time
* number of instanses running for this service at this point.


This is so operation can easy see which executable is a candidate to scale out.

**this is not completed on serverside yet, everything is prepared though**



### transactions

TODO


### casual queue

TODO








