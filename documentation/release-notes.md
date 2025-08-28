![casual](/resources/casual-box-left.png)
# release notes

## 1.8

We're proud to announce the release of casual version `1.8`.

### performance improvements

`casual` now uses epoll on Linux and kqueue on BSD platforms for multiplexing, 
providing more efficient I/O operations compared to previous methods.

### CLI normalization

CLI options have been normalized across all components (gateway, transaction, 
domain, service, queue, file) for consistency and better user experience. 
This includes standardized list options and improved help output.

### file reserve mechanism

Introduced a file reserve mechanism within XA transactions, allowing files to be 
isolated from other transactions when using the API, enhancing data integrity 
in transactional file operations.

### HTTP inbound forwarding

Added support for forwarding HTTP requests directly to services. This includes 
sending raw HTTP data to services, with new nginx directives like `casual_forward_service`
for easy configuration.

### header functionality

Extended header support for service calls and tpcall/tpacall. Users can now 
associate and propagate headers with buffers, enabling better integration with 
external systems.

### refactoring and cleanup

Major refactoring efforts include moving service, server, and transaction contexts 
out of the common module to their respective modules, improving code organization 
and maintainability. Various code cleanups and simplifications have been performed 
throughout.

### logging improvements

Simplified logging by removing verbose levels and reducing the number of log 
categories. This makes debugging easier while reducing log noise.

### other enhancements

- Improved argument help to print only one level for better readability
- Added `--all` flag to `--list-instances` for services to show hidden services
- Enhanced transaction ID handling and cleanup
- Enabled indexing by EU Open Source Software Catalogue


## 1.7

We're proud to announce the release of casual middleware version `1.7`.

### runtime configuration 

`casual` can now update all parts of configuration while running. No part is 
restarted.

A convenient way to update the configuration is using runtime edit.

```console
$ casual configuration --edit
```

see [configuration.operation.md](../middleware/administration/documentation/cli/configuration.operation.md)

### service timeout

The service timeout semantics have changed a bit.

When a service is called that has a timeout, a deadline is set based on the timeout.
This deadline is propagated downstream to consecutively called services.
If a service is invoked with an ongoing deadline, the earliest of the ongoing and 
the timeout based deadline is set for the service. 

Therefore, the first service sets an _execution window_ to which downstream service 
invocations will conform.

One, perhaps negative, consequence of this semantic is that services downstream 
can get timeouts after a fraction of the configured timeout of the downstream
service. However, the changed semantics is more logical for the caller and is easer to comprehend.

### gateway outbound group order

We have added an _order_ configuration for `gateway.outbound.group`. 

All connections within a group, and all groups with the same `order` are treated 
equally. Service calls will be load balanced with _randomization_.
The lowest value for _order_ will be used for a given service.

```yaml
domain:
  gateway:
    outbound:
      groups:
        - order: 10
          connections:
            - address: "a.b.com:5000"
        - order: 10
          connections:
            - address: "a.c.com:5000"
        - order: 2
          connections:
            - address: "a.d.com:5000"
```

see [domain.gateway.operation.md](../middleware/configuration/documentation/domain.gateway.operation.md)

### extended information in service event log

added        | format    | description
-------------|-----------|------------------------
span         | hex       | 64b OpenTelemetry trace span
parent.span  | hex       | 64b OpenTelemetry parent trace span
code.user    | integer   | user return code from tpreturn

We have added `span` and `parent.span` to _event-service-log_. These can be used 
to feed an opentelemetry server to get span telemetry. 

* see [service.log.md](../middleware/event/documentation/service.log.md) 
* see [opentelemetry tracing](https://opentelemetry.io/docs/concepts/signals/traces/) 

We have also added the code that users provide in `tpreturn` to _event-service-log_.
This can be used to filter domain specific errors. For example, `code.result = TPEFAIL` 
might be an expected outcome for a given service. In combination with `code.user` 
additional filtering is possible. 

### branch on inbound

In `1.7` we branch a transaction on the way 'in' to the domain. This change is
primarily because it gives technical advantages, but it can also benefit 
business flows. It is more likely to access a resource with the same branch in 
complex topologies. Sort of a poor mans _tightly coupled_ transactions.

### gateway pair of ipc - tcp sockets

`inbound` and `outbound` tcp connections now have their own _ipc devices_ associated.
This is mostly to make the semantics easier and cleaner. Responsibilities lands 
more naturally where they belong.

This should increase throughput since more control is given to the operating system.

### queue group capacity

Queue groups can now be given a `capacity` property that defines the maximum total size
of all messages on the queue. If a message about to be enqeueued is larger than the remaining
capacity of the group to which the queue belongs, the enqueue will fail.
All the queues can be dequeued even if the group is full.

```yaml
domain:
  queue:
    groups:
      - alias: A
        capacity:
          size: "10KiB"
        queues:
          - name: a
```

see [domain.queue.operation.md](../middleware/configuration/documentation/domain.queue.operation.md)

### disable enqueue/dequeue

This enable users to disable enqueue/dequeue for a given queue. The semantics 
for _disabling_ are equal to 'hiding' the queue for a given action (enqueue/dequeue). 
That is, if a lookup occurs with the action 'enqueue' for a queue that has 
_enqueue_ disabled, the lookup will reply with _no-queue_, as if the queue 
does not exists.

This could be used to manage business flows, especially in conjunction with 
queue forwards.

```yaml
domain:
  queue:
    groups:
      - alias: A
        queues:
          - name: a
            enable:
              enqueue: true
              dequeue: false

```

see [domain.queue.operation.md](../middleware/configuration/documentation/domain.queue.operation.md)

### enable/disable groups

We've introduced enable/disable on configuration groups.

```yaml
domain:
  groups:
    - name: a
    - name: b
      dependencies: [ a]
      enabled: false
    - name: c
      enabled: true
```

This helps turn groups of 'stuff' on and off. 

see [domain.general.operation.md](../middleware/configuration/documentation/domain.general.operation.md)


### CLI moved options

old                                       |  new
------------------------------------------|------------------------------
`casual domain --configuration-get`       | `casual configuration --get`
`casual domain --configuration-post`      | `casual configuration --post`
`casual domain --configuration-edit`      | `casual configuration --edit`
`casual domain --configuration-put`       | `casual configuration --put`
`casual transaction --scale-instances`    | `casual transaction --scale-resource-proxies`


* see [configuration.operation.md](../middleware/administration/documentation/cli/configuration.operation.md)


removed                             |  use
------------------------------------|------------------------------
`casual gateway --list-services`    | `casual service --list-instances`
`casual gateway --list-queues`      | `casual queue --list-queue-instances`


* see [service.operation.md](../middleware/administration/documentation/cli/service.operation.md)
* see [queue.operation.md](../middleware/administration/documentation/cli/queue.operation.md)
* see [transaction.operation.md](../middleware/administration/documentation/cli/transaction.operation.md)



