# configuration build-server

Defines user configuration when building a casual server.

### services

Defines which services the server has (and advertises on startup). The actual `xatmi` conformant
function that is bound to the service can have a different name.

Each service can have different `transaction` semantics:

type         | description
-------------|----------------------------------------------------------------------
automatic    | join transaction if present else start a new transaction (default type)
join         | join transaction if present else execute outside transaction
atomic       | start a new transaction regardless
none         | execute outside transaction regardless

### resources

Defines which `xa` resources to link and use runtime. If a name is provided for a given
resource, then startup configuration phase will ask for resource configuration for that 
given name. This is the preferred way, since it is a lot more explicit.


## Examples

* [build/server.yaml](../sample/build/server.yaml)
* [build/server.json](../sample/build/server.json)
* [build/server.xml](../sample/build/server.xml)
* [build/server.ini](../sample/build/server.ini)
