
# medium multiple domain example

## objective

Explore the abillity of casual to discover resources in remote domains.

We'll set up two domains, **A** and **B**. 
* From within **A** we'll enqueue a message to a queue in **B**. 
* In **B**, a queue-service forward will dequeue the message and call a service in **A** and the reply is enqueued in another queue in **B**.
* in **B**, a queue-queue forward will dequeue the reply and enqueue it at a queue in **A**.
A queue service forward dequeues the message and calls a service that will abort the transaction, hence the message is moved 
to the corresponding error queue.


### diagram

A simplified sequence diagram on what's going on (discovery and transaction related activity is omitted)

![scenario](diagram/scenario.svg)



## prerequisites

See [domain example]( ../../readme.md)


## create domains

Create a directory where you want your domains to "live".

**In production you probably want to have a dedicated user for each domain and just use the "domain-user" home directory as the domain root**

The following will be used in this example.

```bash
>$ mkdir -p $HOME/casual/example/domain/multiple/medium
```
    
Copy the domains setup from the example:

```bash
>$ cp -r $CASUAL_HOME/example/domain/multiple/medium/* $HOME/casual/example/domain/multiple/medium/
```

### configuration

Each domain has it's configuration in `$CASUAL_DOMAIN_HOME/configuration/domain.yaml`. In our example these will be located at:

* `$HOME/casual/example/domain/multiple/medium/domainA/configuration/domain.yaml`
* `$HOME/casual/example/domain/multiple/medium/domainB/configuration/domain.yaml`

_The environment variable_ `CASUAL_DOMAIN_HOME` _is the only thing that dictates which domain you're using and working with_

 * [domainA/configuration/domain.yaml](domainA/configuration/domain.yaml)    
 * [domainB/configuration/domain.yaml](domainB/configuration/domain.yaml) 



If you chose another base directore for this example, please update the following files so they corresponds with your choice
 
 * [domainA/domain.env](domainA/domain.env)    
 * [domainB/domain.env](domainB/domain.env) 
 

## start domainA

In the terminal for domainA.   

### prepare

Make sure the required environment settings are sourced.

*You only have to do this once.*
 
```bash
domainA>$ cd $HOME/casual/example/domain/multiple/medium/domainA
domainA>$ source domain.env
```

### boot

We provide our configuration for the domain:

```bash
domainA>$ casual domain --boot configuration/domain.yaml
``` 

    
## start domainB

In the terminal for domainB.

### prepare

```bash
domainB>$ cd $HOME/casual/example/domain/multiple/medium/domainB
domainB>$ source domain.env
```
### boot

We provide our configuration for the domain:

```bash
domainB>$ casual domain --boot configuration/domain.yaml
```


## interact with the setup


### current state

View current state in the two domains.

#### domain A

List connections:

```bash
domainA>$ casual gateway --list-connections
name               id                                bound  pid    ipc                               runlevel  local            peer           
-----------------  --------------------------------  -----  -----  --------------------------------  --------  ---------------  ---------------
md-medium-domainB  0f8a801678b84bc8b068ba94f9de9a08  out    44779  e83f4bb1d45647c6b15d2623ead42242  online    127.0.0.1:59981  127.0.0.1:7772 
md-medium-domainB  0f8a801678b84bc8b068ba94f9de9a08  in     44793  e3c61a13d97444f5bcc9b5b2de9dcfcf  online    127.0.0.1:7771   127.0.0.1:59980
```

We have one inbound and one outbound connection to `domainB`.



List services:

```bash
domainA>$ casual service --list-services
name                              category  mode  timeout  I  C  AT     min    max    P  PAT    RI  RC  last
--------------------------------  --------  ----  -------  -  -  -----  -----  -----  -  -----  --  --  ----
casual/example/advertised/echo    example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/conversation       example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/echo               example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/error/system       example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/forward/echo       example   auto    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/forward/join/echo  example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/lowercase          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/rollback           example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/sink               example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/sleep              example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/terminate          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/uppercase          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
casual/example/work               example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -   
```


`casual/example/echo` and `casual/example/rollback` is advertised from one **local** instance, and no one has requested the services yet.


List queues:

```bash
domainA>$ casual queue --list-queues 
name           group     rc  rd     count  size  avg  EQ  DQ  UC  last
-------------  --------  --  -----  -----  ----  ---  --  --  --  ----
queueA1        domain-A   0  0.000      0     0    0   0   0   0  -   
queueA1.error  domain-A   0  0.000      0     0    0   0   0   0  -   
```

#### domain B

List connections:

```bash
domainB>$ casual gateway --list-connections 
name               id                                bound  pid    ipc                               runlevel  local            peer           
-----------------  --------------------------------  -----  -----  --------------------------------  --------  ---------------  ---------------
md-medium-domainA  473c23883eac40d0867170cb2fa49830  out    44960  026a2c488d6b4a3d90106cee04f3783a  online    127.0.0.1:60018  127.0.0.1:7771 
md-medium-domainA  473c23883eac40d0867170cb2fa49830  in     44975  64e60b840a8b4e41a10cec6f14a07707  online    127.0.0.1:7772   127.0.0.1:60017
```

We have one inbound and one outbound connection to `domainA`.


List services:

```bash
domainB>$ casual service --list-services
name  category  mode  timeout  I  C  AT  min  max  P  PAT  RI  RC  last
----  --------  ----  -------  -  -  --  ---  ---  -  ---  --  --  ----
```


`casual/example/echo` is not yet known in this domain.


List queues:

```bash
domainB>$ casual queue --list-queues 
name           group     rc  rd     count  size  avg  EQ  DQ  UC  last
-------------  --------  --  -----  -----  ----  ---  --  --  --  ----
queueB1        domain-B   0  0.000      0     0    0   0   0   0  -   
queueB2        domain-B   0  0.000      0     0    0   0   0   0  -   
queueB1.error  domain-B   0  0.000      0     0    0   0   0   0  -   
queueB2.error  domain-B   0  0.000      0     0    0   0   0   0  -   
```


### enqueue a message

In `domainA`, enqueue some characters to `queueB1` that is located in `domainB`

```bash
domainA>$ echo "test" | casual buffer --compose | casual queue --enqueue queueB1
bec3b4b3cccd4f3b89faee970518ab7d
```

The message should be enqueued to `queueA1` and then dequeued and rollbacked, hence end up in `queueA1.error` pretty much directly.

```bash
domainA>$ casual queue --list-queues 
name           group     rc  rd     count  size  avg  EQ  DQ  UC  last                            
-------------  --------  --  -----  -----  ----  ---  --  --  --  --------------------------------
queueA1        domain-A   0  0.000      0     0    0   1   1   0  2020-12-28T21:25:25.169911+01:00
queueA1.error  domain-A   0  0.000      1     5    5   1   0   0  2020-12-28T21:25:25.169911+01:00


The service `casual/example/echo` should be reqeusted once (the call from remote `domainB`).
The service `casual/example/rollback` should be reqeusted once from the forward in this domain.

```bash
domainA>$ casual service --list-services
name                              category  mode  timeout  I  C  AT     min    max    P  PAT    RI  RC  last                            
--------------------------------  --------  ----  -------  -  -  -----  -----  -----  -  -----  --  --  --------------------------------
casual/example/advertised/echo    example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/conversation       example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/echo               example   join    0.000  1  1  0.001  0.001  0.001  1  0.000   0   0  2020-12-28T21:25:25.156694+01:00
casual/example/error/system       example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/forward/echo       example   auto    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/forward/join/echo  example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/lowercase          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/rollback           example   join    0.000  1  1  0.001  0.001  0.001  1  0.000   0   0  2020-12-28T21:25:25.188741+01:00
casual/example/sink               example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/sleep              example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/terminate          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/uppercase          example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -                               
casual/example/work               example   join    0.000  1  0  0.000  0.000  0.000  0  0.000   0   0  -      
```


In `domainB`, `casual/example/echo` should be known with no local instances:

```bash
domainB>$ casual service --list-services
name                 category  mode  timeout  I  C  AT     min    max    P  PAT    RI  RC  last                            
-------------------  --------  ----  -------  -  -  -----  -----  -----  -  -----  --  --  --------------------------------
casual/example/echo  example   join    0.000  0  1  0.005  0.005  0.005  1  0.000   1   1  2020-12-28T21:25:25.158197+01:00
```



