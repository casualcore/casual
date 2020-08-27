# casual queue

```shell
host# casual --help queue

  queue [0..1]
        queue related administration

    SUB OPTIONS
      -q, --list-queues [0..1]
            list information of all queues in current domain

      -r, --list-remote [0..1]
            list all remote discovered queues

      -g, --list-groups [0..1]
            list information of all groups in current domain

      -m, --list-messages [0..1]  (<queue>) [1]
            list information of all messages of a queue

      --restore [0..1]  (<queue>) [0..*]
            restores messages to queue
            
            Messages will be restored to the queue they first was enqueued to (within the same queue-group)
            
            Example:
            casual queue --restore <queue-name>

      -e, --enqueue [0..1]  (<queue>) [1]
            enqueue buffer(s) to a queue from stdin
            
            Assumes a conformant buffer(s)
            
            Example:
            cat somefile.bin | casual queue --enqueue <queue-name>
            
            @note: part of casual-pipe

      -d, --dequeue [0..1]  (<queue>, [<id>]) [1..2]
            dequeue buffer from a queue to stdout
            
            if id is absent the oldest available message is dequeued. 
            
            Example:
            casual queue --dequeue <queue> | <some other part in casual-pipe> | ... | <casual-pipe termination>
            casual queue --dequeue <queue> <id> | <some other part in casual-pipe> | ... | <casual-pipe termination>
            
            @note: part of casual-pipe

      -p, --peek [0..1]  (<queue>, [<id>]) [1..*]
            peeks messages from the give queue and streams them to casual-pipe
            
            Example:
            casual queue --peek <queue-name> <id1> <id2> | <some other part of casual-pipe> | ... | <casual-pipe-termination>
            
            @note: part of casual-pipe

      --consume [0..1]  (<queue>, <count>) [1..2]
            consumes up to `count` messages from the provided `queue` and send it downstream
            
            Example:
            casual queue --consume <queue-name> [<count>] | <some other part of casual-pipe> | ... | <casual-pipe-termination>
            
            @note: part of casual-pipe

      --clear [0..1]  (<queue>) [1..*]
            clears all messages from provided queues
            
            Example:
            casual queue --clear queue-a queue-b

      --remove-messages [0..1]  (<queue>, <id>) [2..*]
            removes specific messages from a given queue

      --metric-reset [0..1]  (<queue>) [0..*]
            resets metrics for the provided queues
            
            if no queues are provided, metrics for all queues are reset.
            
            Example:
            casual queue --metric-reset queue-a queue-b

      --legend [0..1]  (list-queues, list-messages) [1]
            provide legend for the output for some of the options
            
            to view legend for --list-queues use casual queue --legend list-queues.
            
            use auto-complete to help which options has legends

      --information [0..1]
            collect aggregated information about queues in this domain

      --state [0..1]  (json|yaml|xml|ini) [0..1]
            queue state

```