# configuration gateway

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  



## domain.gateway

Defines configuration for communication with other `casual` domains.

### domain.gateway.inbound

Defines all inbound related configuration (from remote domains -> local domain)

#### Limit _(structure)_

property                 | description
-------------------------|------------------------------------------
[size : `integer`]       | max size in bytes
[messages : `integer`]   | max number of messages in flight

#### domain.gateway.inbound.default

Will be used as default values for all groups.

property                                   | description                                       | default
-------------------------------------------|---------------------------------------------------|--------
[limit : `Limit`]                          | default value for limit                           |
[connection.discovery.forward : `boolean`] | if a discovery is allowed to propagate downstream | `false`

##### domain.gateway.inbound.groups.Connection _(structure)_

Defines a connection that this _inbound group_ should listen to

property                        | description                                       | default 
--------------------------------|---------------------------------------------------|---------
address : `string`              | the address to listen on, `host:port`             |         
[discovery.forward : `boolean`] | if a discovery is allowed to propagate downstream | `domain.gateway.inbound.default.connection.discovery.forward`

#### domain.gateway.inbound.groups _(list)_

Defines a list of all inbound groups

property                       | description                           | default
-------------------------------|---------------------------------------|------------
[alias : `string`]             | an _identity_ for this group instance | _generated unique name_
[limit : `Limit`]              | upper limits of inflight messages     | `domain.gateway.inbound.default.limit`
[connections : `[Connection]`] | all the connections for this group    |


### domain.gateway.outbound

Defines all outbound related configuration (from local domain -> remote domains)

##### domain.gateway.outbound.groups.Connection _(structure)_

Defines a connection that this _outbound group_ should try to connect to.

property                 | description
-------------------------|----------------------------------------------------
address : `string`       | the address to connect to, `host:port` 
[services : `[string]`]  | services we're expecting to find on the other side 
[queues : `[string]`]    | queues we're expecting to find on the other side 

`services` and `queues` is used as an _optimization_ to do a _build_ discovery during startup. `casual`
will find these services later lazily otherwise. It can also be used to do some rudimentary load balancing 
to make sure lower prioritized connections are used for `services` and `queues` that could be discovered in
higher prioritized connections.

#### domain.gateway.outbound.groups _(list)_

Each group gets an _order_ in the order they are defined. Groups defined lower down will only be used if the higher
ups does not provide the wanted _service_ or _queue_. Hence, the lower downs can be used as _fallback_.

property                       | description                               | default
-------------------------------|-------------------------------------------|------------
[alias : `string`]             | an _identity_ for this group instance     | _generated unique name_
[order : `integer`]            | a number that is used for load-balancing  | the implied order in the configuration
[connections : `[Connection]`] | all the connections for this group        |

All connections within a group, and all groups with the same `order` ar treated equal. Service calls will be 
load balanced with _randomization_. Although, `casual` will try to _route_ the same transaction to the 
previous _associated_ connection with the specific transaction. This is done to minimize the amount 
of _resources_ involved within the prepare and commit/rollback stage.  


### domain.gateway.reverse

This section defines _reverse_ `inbound` and `outbound`. The connection phase is reversed.
* `outbound` connection listen to it's' configured address.
* `inbound` connections tries to connect to it's configured address.

Otherwise, the semantics and configuration are exactly the same.

### domain.gateway.reverse.inbound

Exactly the same as [domain.gateway.inbound](#domaingatewayinbound)

### domain.gateway.reverse.outbound

Exactly the same as [domain.gateway.outbound](#domaingatewayoutbound)

## examples 

* [domain/gateway.yaml](sample/domain/gateway.yaml)
* [domain/gateway.json](sample/domain/gateway.json)
* [domain/gateway.xml](sample/domain/gateway.xml)
* [domain/gateway.ini](sample/domain/gateway.ini)

