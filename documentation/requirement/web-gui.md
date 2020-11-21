

# Requirements for casual middleware UI

This document tries to define the requirements for what is useful and paramount
for a user of casual middleware

## What we think users wants

There are several *views* that the user wants to see. Some of these can maybe 
be combined.


### groups

View and manage group information

* Name
* Note
* Resources (read only?)
    * xa key, openinfo and such

*Expand to see which executables is member of this group?*

**source: domain-manager**

### executables

We might want to split **servers** and **executables** into two different views.

View and manage exacutable information

List of executables with:

* Alias (this is the external handle of the 'executable')
* Note
* Path
* Instances (read only)
    * Manage number of instances (scale in, scale out)
* Memberships
    * "expand" to group view?
    
*Don't think we'll expose the internal id of the executable, user should only refer to 'alias'. The internal id should be used to correlate instanses though* 
    
**source: domain-manager**
    
### instances

List of instances with:

* pid
* ipc-queue
* Executable
    * "expand" to executable view?

**source: domain-manager**

### services

View and manage exacutable information

List of services with:

* Name
* Type
* transaction-semantic
* Instances
    * "expand" to instance view?
    * if an instance is a outbound-connection (gateway) show info about this.

    
Should be easy to manage instances from this view, since services should be what's interesting 

**source: broker**
 
 
### pending services

view information about pending requests

List of pending services with:

* Name
* Number of requests that have been pending
* Total time that others have waited for this server to be callable
* Calculated average time
* Number of instanses running for this service at this point.


This is so operations can easily see which executable is a candidate to scale out.

**this is not completed on serverside yet, everything is prepared though**



### transactions

TODO


### casual queue

TODO








