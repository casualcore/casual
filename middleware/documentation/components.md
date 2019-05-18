# components 

Aims to give an understanding of the fundamental _components_ that casual consist of.

Each component has a _manager_ that has the responsibility of the _component_

The following diagrim illustrates all _managers_ and their relationsship

![manages](diagram/components.svg "manager relationship")

## domain-manager

Responsibilities:

* start all other managers
* hold domain configuration datastructure
* start all user `servers` and `executables`
* `event` relay/dispatch
* provide _pending message_ service for other _managers_
* notify subscribers when _processes_ terminate/dies 

## service-manager 

Responsibilities:

* service lookup repository 
* any running process that has `services` will advertise these to `service-manager`

## transaction-manager

Responsibilities:

* start all configured `resource proxies`
* coordinate (distributed) transactions
* act as a `resource` to other domains


## gateway-manager

Responsibilities:

* listen to configured addresses and ports
* start configured `outbound` connections
* start `inbound` connections when other domains connect with their `outbound`

#### gateway-outbound 

* forward messages to other end (strict defined protocol)
* act as a resource to `transaction-manager` _(if invocations are in transaction)_


## queue-manager

Responsibilities:

* start all configured `queue-groups`
* queue lookup

#### queue-group 

* provide configured queues
* guaranteed transactional persistent messages _(if configured)_
* act as a `resource` to transaction manager






