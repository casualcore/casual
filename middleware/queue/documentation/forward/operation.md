# queue forward

`casual` has two types of forwards, _forward to a service_ and _forward to a queue_.

The semantics under the hood are _pushing_, hence no activity takes place while
the forward is waiting for a message. 


## forward queue to service

Dequeues from one _queue_ and calls one _service_, with possible _reply-queue_

`casual-queue-forward-service --forward  <queue> <service> [<reply>]`


## forward queue to queue

Dequeues from one _queue_ and enqueue the message to another _queue_

`casual-queue-forward-queue --forward  <from> <to>`


## configuration example

The following contrive example will do the following: 

* Move from `B` to `A`
* Dequeue from `A` and call `casual/example/echo`
* Enqueue result to `B` 

Hence, keep going forever.

``` yaml
domain:
  name: queue-forward-centric-domain-example
  executables: 
    - alias: forward-service
      path: ${CASUAL_HOME}/bin/casual-queue-forward-service
      arguments: [ "--forward", "A", "casual/example/echo", "B"]
  
    - alias: forward-queue
      path: ${CASUAL_HOME}/bin/casual-queue-forward-queue
      arguments: [ "--forward", "B", "A"]

  queue:
    - groups:
        - name: my-group
          queues:
            - name: A
            - name: B

```