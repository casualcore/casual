
# overview of `casual` command line interface

`casual` middleware has one entry point for all it's administration, in a command named `casual`

## semantics

In general the semantics of the `casual CLI` is:

```bash
$ casual <category group> <group specific options>...
```

## help

To get an overview help for what options is possible use:

```bash
$ casual --help
```

To get detailed help for a specific category/option, use:

```bash
$ casual --help service
```

It's possible to get detailed help for several categories/options at once: 

``` shell
$ casual --help service domain gateway
```

To see all possible _help options_ use:

``` shell
$ casual --help --help
```

### help output

`casual CLI` shows the cardinality for the _option_ it self and if the _option_ takes values the
cardinality of the values.

* The cardinality of the _option_ dictates the possible number of usages of that particular 
_option_. A common cardinality for an _option_ is `[0..1]` -> zero-to-one usage (optional).
* The cardinality of the _values_ for an _option_ dictates the number of values that particular
_option_ takes.

semantic                                               | example
-------------------------------------------------------|---------------------------------------------
`<option name> <cardinality>`                          | `--list-servers [0..1]`
`<option name> <cardinality> (<values>) <cardinality>` | `-restart-aliases [0..1]  (<alias>) [1..*]`



notation example  | description
------------------|---------------------------------------------------------------------------------
`[0..1]`          | _zero to one_ 
`[2]`             | exactly 2 occurrences
`[2..*]`          | _two to infinity_
`[1..* {2}]`      | _one to infinity_. if more than `1`, the step is `2`. Possible occurrences: `1, 3, 5, ...`



## bash completion

`casual` provides _bash-auto-completion_ on the command `casual`. Hence, it should help 
users to _tab_ for guidance to appropriate options for their intention. Reducing the amount 
of options one needs to memorize.  

## unix friendly

`casual` aims to be as _unix friendly_ as possible.

What do we mean with _unix friendly_? 

* Every human readable output to stdout should be parsable line by line, hence no composite information
* Users should be able to combine other _unix_ tools to achieve their goals (_grep, sort, cut, |, etc..._) 
* `casual CLI` commands should be as composable as possible

### example

* `casual service --list-services | grep foo | less`
* `casual service --list-services | sort -k6,7 | less`


### colors

`casual CLI` uses colors in output default (can be altered with `--color true/false`).
If `casual CLI` detects that _stdout_ is not bound to a TTY (terminal), no colors will be
produced. This to make it easier to combine `casual CLI` with other unix tools.


### casual-pipe

`casual CLI` has a few _business related_ commands, such as `queue --enqueue/--dequeue`, `call --service`,
`transaction --begin/--commit`, that could be used to solve real business problems. 

These cli commands are annotated with 'casual-pipe' in the help.

These _business related_ commands communicate with a non human readable 'internal' _pipeline-protocol_ (binary) 
via _stdout -> stdin_, hence it's possible to compose them.

The 'causal-pipe' has to be _terminated_ to be able to consume `stdout` with cli commands that are not part
of 'casual-pipe'.

If `casual` detects that `stdout` is tied to a _terminal_ `casual` will try to make it _human readable_

#### example

```bash
$ echo "some payload" | casual buffer --compose "X_OCTET/" | casual buffer --duplicate 2 | casual buffer --extract
```
* `casual buffer --compose "X_OCTET/"` creates an `X_OCTET/` buffer with the payload 'some payload' and send it downstream
* `casual buffer --duplicate 2` duplicates the buffer twice and send it downstream
* `casual buffer --extract` extracts the payload of the buffer and sends it downstream, which is the terminal in this case.
   * If the payload is human readable, the output will be human readable.

```bash
host# casual transaction --begin \
 | casual queue --dequeue b2 \
 | casual call --service casual/example/echo \
 | casual queue --enqueue a2 \
 | casual transaction --commit
```

* `casual transaction --begin` starts a _transaction_ that will associate all actions with the _transaction_ downstream    
* `casual queue --dequeue b2` dequeues a message from `b2` in _transaction_ and send it downstream
* `casual call --service casual/example/echo` calls service with payload from `dequeue` in _transaction_, the service 
    reply is sent downstream.
* `casual queue --enqueue a2` enqueues the service reply to `a2` in _transaction_
* `casual transaction --commit` commits the `transaction` 

##### attention

If a transaction is used in the cli it's paramount to terminate the transaction directive with `casual transaction --commit`
or `casual transaction --rollback`.
Otherwise the transaction(s) will never be committed/rolled back, and manually recovery is needed.
The information/data will be safe and protected by the transaction semantics though, so no data will 
be lost.

  
## categories

A complete list of all _categories_ and a brief description

### domain

Responsible for the current local domain. Scales servers and executables, and the
lifetime of the domain.

See [domain.operation.md](domain.operation.md)

### service

Responsible for all the services in the local domain.

See [service.operation.md](service.operation.md)

### gateway

Responsible for all inbound and outbound connections for the local domain.

See [gateway.operation.md](gateway.operation.md)

### transaction

Administration of transactions.

See [transaction.operation.md](transaction.operation.md)

### queue

Administration for `casual-queue`.

See [queue.operation.md](queue.operation.md)

### call

A generic `XATMI` service caller

See [call.operation.md](call.operation.md)

### describe

A `casual` specific service describer

See [describe.operation.md](describe.operation.md)