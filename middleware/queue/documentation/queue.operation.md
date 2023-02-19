# casual-queue

`casual-queue` is a simple and lightweight _queue manager_.


`casual-queue` differs from other _xa-resources_ in the form that `casual-queue` is 'known' to `casual`, 
hence `casual` can, and does, take shortcuts regarding transaction semantics. User code does not have 
to bind/link to `casual-queue` as a _xa-resource_, it works more like _service calls_ 

## groups

`casual-queue` groups _queues_ into _groups_, both logically and physically. This enables possibilities to
have different storage for different _groups_ (and their _queues_).

_group names_ can be anything as long as they're unique within the `domain`.

### queuebase
`casual-queue` stores the _group_ in a _queuebase_ file (a sqlite database file) at the configured path.
You can specify where the _group_ will be stored, via `domain/queue/group/queuebase: /some/fast/disk/group-a.qb`.

Default _queuebase file_ for a _group_ is `$CASUAL_DOMAIN_HOME/queue/<group-name>.qb`

### in-memory

_groups_ can be configured to have a _non persistance in memory storage_, this is done with the magical name `:memory:` 
(this is how `sqlite` works, so this is more a consequence of the choice to use `sqlite` as _storage manager_).


## queues

Every _queue_ gets a corresponding _error-queue_, which takes the name: `<queue-name>.error`

_queues_ can be named to anything as long as they're unique within the `domain`, this includes the generated error queues.
Hence, it's not possible to create `foo` _and_ `foo.error`, since the latter will collide with the generated.

_queues_ can be configured to have `retry.count` and `retry.delay`:

* `retry.count`: how many times a message can be dequeued and rolledback before the message is moved to the _error queue_
* `retry.delay`: how long time should the message be _not available_ after a dequeue and rollback.

_error-queues_ can be accessed exactly like any other _queue_. The only difference is that `retry`
will always be `retry.count = 0` and `retry.delay = 0s`, or rather, the `retry` will not be taken into account for _error-queues_. 

The only way to remove messages from an _error-queue_ is to consume the messages (dequeue [successful commit]), if a rollback
takes place the message will remain on the _error-queue_. 

### clear

To clear a queue, that is, remove and discard messages, use 
```bash
$ casual queue --consume <some-queue> > /dev/null
```

## configuration

**example**
``` yaml
 
domain:
  name: queue-centric-domain-example

  queue:
    default:
      queue:
        retry:
          # number of times a message can be rolled back before it will be moved to 
          # error queue
          count: 3 

          # duration until a rolled backed message is available for consumption (SI-units: h, min, s, ms, us, ns)
          delay: 20s


      directory: ${CASUAL_DOMAIN_HOME}/queue/groups

    groups:
      - name: groupA
        note: "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb"
        queues:
          - name: q_A1

          - name: q_A2
            retry:
              count: 10
              delay: 100ms
            note: after 10 rollbacked dequeues, message is moved to q_A2.error

          - name: q_A3
          - name: q_A4

      - name: groupB
        queuebase: /some/fast/disk/queue/groupB.qb
        queues:
          - name: q_B1
          - name: q_B2
            retry:
              count: 20

            note: after 20 rollbacked dequeues, message is moved to q_B2.error. retry.delay is 'inherited' from default, if any

      - name: groupC
        queuebase: ":memory:"
        note: group is an in-memory queue, hence no persistence
        queues:
          - name: q_C1

          - name: q_C2

```

For a more accurate configuration example (generated example), see [domain][queue-configuration-example]





[queue-configuration-example]: ../../configuration/example/domain/domain.yaml