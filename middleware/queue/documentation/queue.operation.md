# casual-queue

`casual-queue` is a simple and lightweight _queue manager_.


`casual-queue` differs from other _xa-resources_ in the form that `casual-queue`
is 'known' to `casual`, hence `casual` can, and does, take shortcuts regarding
transaction semantics. User code does not have to bind/link to `casual-queue` as
a _xa-resource_, it works more like _service calls_

## groups

`casual-queue` groups _queues_ into _groups_, both logically and physically.
This enables possibilities to have different storage for different _groups_ (and
their _queues_).

_group names_ can be anything as long as they're unique within the `domain`.

### queuebase
`casual-queue` stores the _group_ in a _queuebase_ file (a sqlite database file)
at the configured path. You can specify where the _group_ will be stored, via
`domain/queue/group/queuebase: /some/fast/disk/group-a.qb`.

Default _queuebase file_ for a _group_ is `$CASUAL_DOMAIN_HOME/queue/<group-name>.qb`

### in-memory

_groups_ can be configured to have a _non persistance in memory storage_, this
is done with the magical name `:memory:` (this is how `sqlite` works, so this is
more a consequence of the choice to use `sqlite` as _storage manager_).


## queues

Every _queue_ gets a corresponding _error-queue_, which takes the name:
`<queue-name>.error`

_queues_ can be named to anything as long as they're unique within the `domain`,
this includes the generated error queues. Hence, it's not possible to create
`foo` _and_ `foo.error`, since the latter will collide with the generated.

_queues_ can be configured to have `retry.count` and `retry.delay`:

* `retry.count`: how many times a message can be dequeued and rolledback before
  the message is moved to the _error queue_
* `retry.delay`: how long time should the message be _not available_ after a
  dequeue and rollback.

_error-queues_ can be accessed exactly like any other _queue_. The only
difference is that `retry` will always be `retry.count = 0` and
`retry.delay = 0s`, or rather, the `retry` will not be taken into account for
_error-queues_.

The only way to remove messages from an _error-queue_ is to consume the messages
(dequeue [successful commit]), if a rollback takes place the message will remain
on the _error-queue_.


## configuration

see [configuration.domain.queue.operation](../../configuration/documentation/domain.queue.operation.md)

