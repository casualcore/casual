# Medium multiple domain example

## Table of Contents

<!-- toc -->

- [Objective](#markdown-header-objective)
- [Create domains](#markdown-header-create-domains)
  * [Configuration](#markdown-header-configuration)
- [Start domains](#markdown-header-start-domains)
  * [Domain A](#markdown-header-domain-a)
  * [Domain B](#markdown-header-domain-b)
- [Interact with the domains](#markdown-header-interact-with-the-domains)
  * [Current state](#markdown-header-current-state)
    + [Domain A](#markdown-header-domain-a-1)
    + [Domain B](#markdown-header-domain-b-1)
  * [Enqueue a message](#markdown-header-enqueue-a-message)

<!-- tocstop -->

## Objective

Explore the abillity of casual to discover resources in remote domains.

We'll set up two domains, **A** and **B**. Then we'll do the following:

1. **A** will enqueue a message to a queue in **B**. 

2. In **B** a queue-service forward will dequeue the message and call a service in **A** and the reply is enqueued in another queue in **B**. 

3. In **B** a queue-queue forward will dequeue the reply and enqueue it at a queue in **A**.

4. A queue service forward dequeue the message and call a service that will abort the transaction, hence the message is moved 
to the corresponding error queue.


A simplified sequence diagram on what's going on (discovery and transaction related activity is omitted).

![scenario](./diagram/scenario.svg)

## Create domains

Create a directory where you want your domains to "live".

In a production environment one probably wants to have a dedicated user for each domain and just use the "domain-user" home directory as the domain root.

Copy the domain setup from the example:

```bash
cd <your domains root directory>
cp -r $CASUAL_HOME/example/domain/multiple/medium/domain* .
```

### Configuration

Each domain has it's configuration in `$CASUAL_DOMAIN_HOME/configuration/domain.yaml`. The environment variable `CASUAL_DOMAIN_HOME` is the only thing that 
dictate which domain you're using and working with. 

If you chose another base directory for this example, please update `domain.env` for each domain.

| Domain | Configuration                            | Environment               |
| ------ | ---------------------------------------- | ------------------------- |
| **A**  | `$PWD/domainA/configuration/domain.yaml` | `$PWD/domainA/domain.env` |
| **B**  | `$PWD/domainB/configuration/domain.yaml` | `$PWD/domainB/domain.env` |

## Start domains

Open up two terminal windows, of your choice.

### Domain A

Domain **A** is going to be started in the first terminal.

Make sure we have the _few_ requered environment settings sourced.

You only have to do this once, of course.
 
```bash
cd domainA/
. domain.env
casual-admin domain --boot configuration/domain.yaml
``` 
    
### Domain B

Domain **B** is going to be started in the second terminal.


```bash
cd domainB/
. domain.env
casual-admin domain --boot configuration/domain.yaml
```

## Interact with the domains

### Current state

View current state in the two domains.

#### Domain A

List connections with `casual-admin gateway --list-connections`. Example output:

```
name               id                                bound  pid    queue    type  runlevel  address        
-----------------  --------------------------------  -----  -----  -------  ----  --------  ---------------
md-medium-domainB  b0cf47002d4642f7a72913d40cde6a92  out    22351  8060933  tcp   online    localhost:7772 
md-medium-domainB  b0cf47002d4642f7a72913d40cde6a92  in     22376   917521  tcp   online    localhost:64495
```

We have one inbound and one outbound connection to `domainB`.

List services with `casual-admin broker --list-services`. Example output:

```
name                     type  mode  timeout  invoked  local  load    avg T   tot pending #  avg pending T  remote
-----------------------  ----  ----  -------  -------  -----  ------  ------  -------------  -------------  ------
casual.example.echo         0  join   0.0000        0      1  0.0000  0.0000              0         0.0000       0
casual.example.rollback     0  join   0.0000        0      1  0.0000  0.0000              0         0.0000       0
casual.example.sink         0  join   0.0000        0      1  0.0000  0.0000              0         0.0000       0
```

`casual.example.echo` and `casual.example.rollback` is advertised from one **local** instance (`remote = 0`), and no one has 
requested the services yet (`invoked = 0`).

List queus with `casual-admin queue --list-queues`. Example output:

```
name                  count  size  avg  uc  updated                  r  error queue           group   
--------------------  -----  ----  ---  --  -----------------------  -  --------------------  --------
queueA1                   0     0    0   0  2016-11-20T15:17:34.068  0  queueA1.error         domain-A
queueA1.error             0     0    0   0  2016-11-20T15:17:34.068  0  domain-A.group.error  domain-A
domain-A.group.error      0     0    0   0  2016-11-20T15:17:34.062  0  domain-A.group.error  domain-A
```

#### Domain B

List connections with `casual-admin gateway --list-connections`. Example output:

```
name               id                                bound  pid    queue   type  runlevel  address        
-----------------  --------------------------------  -----  -----  ------  ----  --------  ---------------
md-medium-domainA  653081bee12347029d207e8d131cd68f  out    22375  917517  tcp   online    localhost:7771 
md-medium-domainA  653081bee12347029d207e8d131cd68f  in     22377  720916  tcp   online    localhost:64496
```

We have one inbound and one outbound connection to `domainA`.

List services with `casual-admin broker --list-services`. Example output:

```
name                                  type  mode  timeout  requested  local  busy  pending  load   remote
------------------------------------  ----  ----  -------  ---------  -----  ----  -------  -----  ------
```

`casual.example.echo` is not yet known in this domain.

List queues with `casual-admin queue --list-queues`. Example output:

```
name                  count  size  avg  uc  updated                  r  error queue           group   
--------------------  -----  ----  ---  --  -----------------------  -  --------------------  --------
queueB1                   0     0    0   0  2016-11-20T15:17:36.912  0  queueB1.error         domain-B
queueB2                   0     0    0   0  2016-11-20T15:17:36.914  0  queueB2.error         domain-B
queueB1.error             0     0    0   0  2016-11-20T15:17:36.912  0  domain-B.group.error  domain-B
queueB2.error             0     0    0   0  2016-11-20T15:17:36.914  0  domain-B.group.error  domain-B
domain-B.group.error      0     0    0   0  2016-11-20T15:17:36.904  0  domain-B.group.error  domain-B
```

### Enqueue a message

In `domainA`, enqueue some characters to `queueB1` that is located in `domainB`, this is done with `casual-admin queue --enqueue <queue>`.

Example:
```bash
$ echo "test" | casual-admin queue --enqueue queueB1
bec3b4b3cccd4f3b89faee970518ab7d
```

The message should be enqueued to `queueA1` and then dequeued and rollbacked, and then end up in `queueA1.error` more or less
straight away. 

Check the queues on `domainA` with `casual-admin queue --list-queues`. Example output:

```
name                  count  size  avg  uc  updated                  r  error queue           group   
--------------------  -----  ----  ---  --  -----------------------  -  --------------------  --------
queueA1                   0     0    0   0  2016-12-17T14:45:13.119  0  queueA1.error         domain-A
queueA1.error             1     6    6   0  2016-12-17T14:42:53.620  0  domain-A.group.error  domain-A
domain-A.group.error      0     0    0   0  2016-12-17T14:42:53.614  0  domain-A.group.error  domain-A
```

The service `casual.example.echo` should've been reqeusted once (the call from remote `domainB`). The service `casual.example.rollback` 
should've been reqeusted once from the forward in this domain (`domainA`). 

Check the services on `domainA` with `casual-admin broker --list-services`. Example output:

```
name                     type  mode  timeout  invoked  local  load    avg T   tot pending #  avg pending T  remote
-----------------------  ----  ----  -------  -------  -----  ------  ------  -------------  -------------  ------
casual.example.echo         0  join   0.0000        1      1  0.0000  0.0002              0         0.0000       0
casual.example.rollback     0  join   0.0000        1      1  0.0000  0.0002              0         0.0000       0
casual.example.sink         0  join   0.0000        0      1  0.0000  0.0000              0         0.0000       0
```

In `domainB`, the service `casual.example.echo` should be known with no local instances. 

Check services on `domainB` with `casual-admin broker --list-services`. Example output:

```
name                 type  mode  timeout  invoked  local  load    avg T   tot pending #  avg pending T  remote
-------------------  ----  ----  -------  -------  -----  ------  ------  -------------  -------------  ------
casual.example.echo     0  join   0.0000        1      0  0.0000  0.0000              0         0.0000       1
```
