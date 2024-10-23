# configuration queue

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  



## domain.queue

Defines the queue configuration

### Duration : `string`

A string representation of a _duration_. SI units can be used. Example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used


### Retry _(structure)_

property               | description                                                  | default
-----------------------|--------------------------------------------------------------|-----------
[count : `integer`]    | number of rollbacks before move to _error queue_             | `0`
[delay : `Duration`]   | delay before message is available for dequeue after rollback | `0s`

### Enable _(structure)_

property               | description                            | default
-----------------------|----------------------------------------|-----------
[enqueue : `boolean`]  | if enqueue is enabled for the queue    | `true`
[dequeue : `boolean`]  | if dequeue is enabled for the queue    | `true`

### domain.queue.default

Default properties that will be used if not defined per `queue`

property                 | description                           | default
-------------------------|---------------------------------------|------------
[directory : `string`]   | directory for generated storage files | `${CASUAL_PERSISTENT_DIRECTORY}/queue` 
[queue.retry : `Retry`]  | retry semantics                       |

### Bytes : `string`

A string representation of a quantity of _bytes_. IEC 80000-13 units up to T/Ti can be used. Example: `3B`, `24MiB`, `15MB`. If no unit is specified, `B` is used.

### Capacity _(structure)_

property               | description                                                 | default
-----------------------|-------------------------------------------------------------|-----------
[size : `Bytes`]       | the maximum total size of messages that the group may store |

### domain.queue.groups _(list)_

Defines groups of queues which share the same storage location. Groups has no other meaning.

property                | description                                  | default
------------------------|----------------------------------------------|------
alias : `string`        | the (unique) alias of the group.             | 
[queuebase : `string`]  | the path to the storage file.                | `domain.queue.default.directory/<group-name>.qb`
[queues : `list`]       | defines all queues in this group, see below  |
[capacity : `capacity`] | limits the storage capacity of the group     |


Note: if `:memory:` is used as `queuebase`, the storage is non persistent


#### domain.queue.groups.queues

property             | description
---------------------|----------------------------------------------------
name : `string`      | the (unique) name of the queue.
[retry : `Retry`]    | retry semantics
[enable : `Enable`]  | enqueue/dequeue enable/disable


### domain.queue.forward

Section to configure queue to service forwards and queue to queue forwards.


#### domain.queue.forward.default

Default properties that will be used if not explicitly configured per forward.

property                           | description                   | default
-----------------------------------|-------------------------------|------------
[service.instances : `integer`]    | number of forward instances   | `1`
[service.reply.delay : `Duration`] | reply enqueue available delay | `0s`
[queue.instances : `integer`]      | number of forward instances   | `1`
[queue.target.delay : `Duration`]  | enqueue available delay       | `0s`   


##### domain.queue.forward.groups _(list)_

property             | description
---------------------|----------------------------------
alias : `string`     | the (unique) alias of the group. 
[services : `list`]  | queue-to-services forwards
[queues : `list`]    | queue-to-queue forwards 


##### domain.queue.forward.groups.services _(list)_

property                   | description                         | default
---------------------------|-------------------------------------|----------------------------------------
source : `string`          | the queue to dequeue from           |
[alias : `string`]         | the (unique) alias of the forward   | `<source>`, on collisions `<source>.#`
[instances : `integer`]    | number of multiplexing 'instances'  | `domain.queue.forward.default.service.instances`
target.service : `string`  | service to call                     |
[reply.queue : `string`]   | queue to enqueue reply to           |
[reply.delay : `Duration`] | reply enqueue available delay       | `domain.queue.forward.default.service.reply.delay`

##### domain.queue.forward.groups.queues _(list)_

property                    | description                         | default
----------------------------|-------------------------------------|----------------------------------------
source : `string`           | the queue to dequeue from           |
[alias : `string`]          | the (unique) alias of the forward   | `<source>`, on collisions `<source>.#`
[instances : `integer`]     | number of multiplexing 'instances'  | `domain.queue.forward.default.queue.instances`
target.queue : `string`     | queue to enqueue to                 |
[target.delay : `Duration`] | enqueued available delay            | `domain.queue.forward.default.queue.target.delay`


##### domain.queue.forward.groups.queues _(list)_

property                    | description                         | default
----------------------------|-------------------------------------|----------------------------------------
source : `string`           | the queue to dequeue from           |
[alias : `string`]          | the (unique) alias of the forward   | `<source>`, on collisions `<source>.#`
[instances : `integer`]     | number of multiplexing 'instances'  | `domain.queue.forward.default.queue.instances`
target.queue : `string`     | queue to enqueue to                 |
[target.delay : `Duration`] | enqueued available delay            | `domain.queue.forward.default.queue.target.delay`

## examples 

* [domain/queue.yaml](sample/domain/queue.yaml)
* [domain/queue.json](sample/domain/queue.json)
* [domain/queue.xml](sample/domain/queue.xml)
* [domain/queue.ini](sample/domain/queue.ini)

