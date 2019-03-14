# user particles

Aims to give a view of the fundamental user _particles_ of `casual` and their relationships.

That is, the entities that are involved from a user code perspective, during runtime.

## particles

The following diagram illustrates the relationship between the user particles 

![particles](diagram/user-particles.svg "particle relationship")

### resource

* configuration of a specific resource
* `servers` and `executables` uses this to "connect" to resources (if they're _built_ with a resource)

### group

* Groups `server` and `executables`. 
* dependencies to other `groups`
* determine the _boot_ and _shutdown_ ordering
* has `0..*` `resources` _dependencies_, which give _members_ of the group _implicit dependencies_


### server

* _executable_ that `casual` can communicate whith, i.e. it has a _message pump_.
* scales with `instances`
* is member of `0..*` `groups`.
* has `0..*` explicit `resources` _dependencies_ 

### instance (server)

* running process of a given `server`
* advertises `0..*` `services`
* is member of `0..*` `groups`.

### executable 

* arbitary _executable_
* `casual` communicates only with `SIGINT` to shutdown the _executable_
* has `0..*` explicit `resources` _dependencies_ 

### instance (executable)

* running process of a given `executable`

### service

* `XATMI` service
* _entry point_ for `0..*` `instances` (that could be from different `servers`)






