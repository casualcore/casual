# queue forward

`casual` has two types of forwards, _forward to service_ and _forward to queue_.

The semantics under the hood is _pushing_, hence no activity takes place while
the forward waiting for a message. 


## forward queue to service

dequeues from one _queue_ and calls one _service_, with possible _reply-queue_

`casual-queue-forward-service --forward  <queue> <service> [<reply>]`


## forward queue to queue

dequeues from one _queue_ and enqueue the message to another _queue_

`casual-queue-forward-queue --forward  <from> <to>`


## configuration example

the following contrive example will do the following: 

* move from `B` to `A`
* dequeue from `A` and call `casual/example/echo`
* enqueue result to `B` 

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
    - name: my-group
      queues:
         - name: A
         - name: B

```