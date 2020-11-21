# common

Responsible for anything that is common between other parts of casual.


## Design

### Global state

Since the XATMI spec is plain C-functions we need to keep global state.

Every part that needs global state implements a 'context', which is just a singleton. 
With this we can keep the state together with the implementation.

We could, in the future, lock access to these 'contexts' if we want to support users using XATMI 
functions from different threads. But since we're not big fans of threads in genereal, especially in
business code, this is highly unlikely to happen.

*The user can use as many threads as he or she wants, as long as XATMI-interaction is done from the main thread*


### Threads 
We try to avoid threads as much as possible, since it's way to error prone and hard to reason about.

We only use threads in mockups for unit testing, and we'll probably use threads in the gateway.

Hence, very few parts are thread safe.


### Redesign
The following parts needs a redesign

* call
* transaction
* server

My (Fredrik) intention was that these should and could be isolated and not know of each other, at least no
circular dependencies. I was wrong... 

As of now, there is no clear responsibillity between these guys. And if it's not possible to make it so, we
should merge these into one 'context', so it at least is clear from the outside. 
 




