
# overview of `casual` command line interface

`casual` middleware has one entry point for all it's administration, in a command named `casual`

## semantics

In general the semantics of the `casual` `CLI` is:

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

`casual cli` shows the cardinality for the _option_ it self and if the _option_ takes values the
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
of options you needs to memorize.  

## unix friendly

`casual` aims to be as _unix friendly_ as possible.

What do we mean with _unix friendly_? 

* Every human readable output to stdout should be parsable line by line, hence no composite information
* Users should be able to combine other _unix_ tools to achieve their goals (_grep, sort, cut, |, etc..._) 
* `casual` cli commands should be as composable as possible

### example

* `casual service --list-services | grep foo | less`
* `casual service --color false --header false --list-services | sort -k6,7 | less`

_sort doesn't like terminal color (pre|suf)fixes..._ 

### casual-pipe

To enable true transaction support in a cli context `casual` has its own 'internal' _pipeline-protocol_.

This only applies for cli commands that consumes/invokes buffer/queues/services - hence, business execution.
These cli commands are annotated with 'casual-pipe' in the help.

The 'causal-pipe' has to be _terminated_ to be able to consume `stdout` with cli commands that are not part
of 'casual-pipe'.

If `casual` detects that `stdout` is tied to a _terminal_ `casual` will try to make it _human readable_

#### example

```bash
$ echo "some payload" | casual buffer --compose "X_OCTET/" | casual buffer --duplicate 2 | casual buffer --extract
```
* Creates an `X_OCTET/`  buffer with the payload 'some payload' and send it downstream
* Duplicates this buffer twice and send it downstream
* Extracts the payload of the buffer and sends it downstream, which is the terminal in this case.
   * In ths case the payload is human readable.



```bash
host# casual transaction --compound \
 | casual queue --consume b2 \
 | casual call --service casual/example/echo \
 | casual queue --enqueue a2 \
 | casual transaction --commit
```

* Starts a _transaction directive_ that will associate all actions with a new transaction, and sends this downstream
    * Starts waiting on transactions to commit/rollback from downstream
* Consumes all messages on queue `b2`, each _dequeue_ in a separate transaction according to the _directive_
    * Each message (payload) will be associated with its transaction and sent downstream  
* Calls the service `casual/example/echo` with each payload and associated transaction
    * The reply (payload) will have the original transaction associated with it, and sent downstream
    * If the service fails (rollback) the individual transaction will be set to _abort only_
* Enqueue's each payload with associated transaction (which might be in _abort only_)
    * The message id with associated transaction is sent downstream
* Commits all _commitable_ transactions (and rollback the _uncomittable_)
    * Sends notification for each transaction to the owner of the transaction (in this case `casual transaction --compound`)
        * `casual transaction --compound` is the cli command that actually commits or rollback each transaction (in this example)
    * Terminates the transaction-directive by sending a _cli-transaction-termination-message_ to `casual transaction --compound`
        * `casual transaction --compound` knows that it's work is done, and can exit. 
    * The above is done with internal _ipc communication_ 
    * Sends message id's downstream, without the associated transaction (since it has been completed)
    * In this case, `casual` detects that `stdout` is tied to a terminal and 'transform' the id's to be human readable


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