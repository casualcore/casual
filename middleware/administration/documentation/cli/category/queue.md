# queue `casual CLI`

```
  queue  0..1
        queue related administration

    SUB OPTIONS
      -q, --list-queues  0..1
            list information of all queues in current domain

      -r, --list-remote  0..1
            list all remote discovered queues

      -g, --list-groups  0..1
            list information of all groups in current domain

      -m, --list-messages  0..1  (<value>) [1]
            list information of all messages of a queue

      --restore  0..1  (<value>...) [0..*]
            restores messages to queue
            
            that has been rolled back to error queue
              casual queue --restore <queue-name>

      -e, --enqueue  0..1  (<value>) [1]
            enqueue to a queue from stdin
            
              cat somefile.bin | casual queue --enqueue <queue-name>
              note: operation is atomic

      -d, --dequeue  0..1  (<value>) [1]
            dequeue from a queue to stdout
            
              casual queue --dequeue <queue-name> > somefile.bin
              note: operation is atomic

      --state  0..1  ([json,yaml,xml,ini]) [0..1]
            queue state
```