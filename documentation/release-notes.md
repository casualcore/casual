# casual release notes

## 1.7

We're proud to bla bla bla...

### runtime configuration 

`casual` can now update all parts of configuration while running. No part is 
restarted.

A convenient way for a user to update configuration is to runtime edit.

```console
$ casual configuration --edit
```

### service timeout

The service timeout semantics have change a bit.

When a service is called that has a timeout, a deadline is set based on the timeout.
This deadline is propagated downstream to consecutive called services.
If a service is invoked with an ongoing deadline, the earliest of the ongoing and 
the timeout based deadline is set for the service. 

Hence, the first service sets an _execution window_ that downstream service 
invocations will conform to.

One, perhaps negative, consequence of this semantic is that services downstream 
could get timeouts after a fraction of the configured timeout of the downstream
service.

### branch on inbound

In `1.7` we branch a transaction on the way 'in' to the domain. This change is
primarily because it gives technical advantages, but it could also benefit 
business flows. It is more likely to access a resource with the same branch in 
complex topologies. Sort of a poor mans _tightly coupled_ transactions.

### gateway pair of ipc - tcp sockets

`inbound` and `outbound` tcp connections now have their own _ipc devices_ associated.
This is mostly to make the semantics easier and cleaner. Responsibilities lands 
more naturally where they belong.

This should increase throughput since more control is given to the operating system.

### disable enqueue/dequeue

This enable users to disable enqueue/dequeue for a given queue. The semantics 
for _disabling_ are equal to 'hide' the queue for a given action (enqueue/dequeue). 
That is, if a lookup occur with the action 'enqueue' for a queue that has 
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

This should help turn groups of stuff on and off. 

see [domain.general.operation.md](../middleware/configuration/documentation/domain.general.operation.md)


### CLI moved options

old                                       |  new
------------------------------------------|------------------------------
`casual domain --configuration-get`       | `casual configuration --get`
`casual domain --configuration-post`      | `casual configuration --post`
`casual domain --configuration-edit`      | `casual configuration --edit`
`casual domain --configuration-put`       | `casual configuration --put`
`casual transaction --scale-instances`    | `casual transaction --scale-resource-proxies`



removed                             |  use
------------------------------------|------------------------------
`casual gateway --list-services`    | `casual service --list-instances`
`casual gateway --list-queues`      | `casual queue --list-instances`


* see [service.operation.md](../middleware/administration/documentation/cli/service.operation.md)
* see [queue.operation.md](../middleware/administration/documentation/cli/queue.operation.md)


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
