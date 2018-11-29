
# overview of `casual` command line interface

`casual` middleware has one entry point for all it's administration, in a command named `casual`

## semantics

In general the semantics of the `casual` `CLI` is:

```
casual <category group> <group specific options>...
```

## unix friendly

`casual` aims to be as _unix friendly_ as possible.

What do we mean with _unix friendly_? 
* every output to stdout should be parsable line by line, hence no composite information
* users should be able to combine other _unix_ tools to achieve their goals (_grep, sort, cut, |, etc..._) 
* `casual` cli commands should be as composable as possible
  * example: `casual queue --dequeue q1 | casual call service1 | casual queue --enqueue q2` 

## help

To get an overview help for what options is possible use:

``` bash
host$ casual --help
```

To get detailed help for a specific category/option, use:

``` bash
host$ casual --help service
```

It's possible to get detailed help for several categories/options at once 

``` bash
host$ casual --help service domain gateway
```

To see all possible _help options_ use:

``` bash
host$ casual --help --help
```

## bash completion

`casual` provides _bash-auto-completion_ on the command `casual`. Hence, it should help 
users to _tab_ guidance to appropriate options for their intention. It will certainly 
reduce the amount of options one needs to memorize.    

## categories

A complete list of all _categories_ and a brief description

### domain

Responsible for the current local domain. Scales servers and executable, and the
lifetime of domain.

see [domain.md](category/domain.md)

### service

Responsible for all the services in the local domain.

see [service.md](category/service.md)

### gateway

Responsible for all inbound and outbound connections for the local domain.

see [gateway.md](category/gateway.md)

### transaction

Administration of transactions.

see [transaction.md](category/transaction.md)

### queue

Administration for `casual-queue`.

see [queue.md](category/queue.md)


### call

A generic `XATMI` service caller

see [call.md](category/call.md)

### describe

A `casual` specific service describer

see [describe.md](category/describe.md)