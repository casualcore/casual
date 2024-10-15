# casual release notes

## 1.7

We're proud to bla bla bla...

### runtime configuration 

`casual` can now update all parts of configuration while running. No part is 
restarted.

A convenient way for a user to update configuration is to runtime edit.

```bash
$ casual configuration --edit
```


### branch on inbound

In `1.7` we branch a transaction on the way 'in' to the domain. This change is
primarily because it gives technical advantages, but it could also benefit 
business flows. It is more likely to access a resource with the same branch in 
complex topologies. Sort of a poor mans _tightly coupled_ transactions.

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

### enable/disable groups

We've introduced enable/disable on configuration groups.

```yaml
domain:
  groups:
    - name: a
    - name: b
      dependencies: [ a]

```







