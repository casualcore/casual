# casual Operations Tutorial

## 1 Introduction

This tutorial will cover the basics of casual from an operations perspective. We will go over how to configure a basic domain, how to add services and queues as well as setting up connections between different domains. More development-oriented topics like writing and building servers are outside the scope of this tutorial.

It is assumed that you already have a working installation of casual, as well as some basic familiarity with Linux. To help you follow along, every command you will need to execute will be shown along with the expected output.

## 2 Setup

### 2.1 Prerequisites

Before we get started, we need to confirm that everything is installed and that you can access the casual CLI. Try running the command  `casual --version`. You should see output similar to the following:

```bash
$ casual --version
key       value
--------  ----------------------------------------
casual    1.6.16
commit    aaa9becd066149dc7ea6cd93985ecfc4c39f3a51
compiler  g++: 9.3.1
```

If you get an error, ensure that the `CASUAL_HOME` environment variable points to the location of your casual installation, that `$CASUAL_HOME/bin` is in your `PATH` and that `$CASUAL_HOME/lib` is in your `LD_LIBRARY_PATH`, for example:

```bash
$ export CASUAL_HOME=/opt/casual
$ export PATH=$CASUAL_HOME/bin:$PATH
$ export LD_LIBRARY_PATH=$CASUAL_HOME/lib:$LD_LIBRARY_PATH
```

### 2.2 Environment

casual domains use several different environment variables to define their operation, the most important of which is `CASUAL_DOMAIN_HOME`. This specifies the home directory of the domain and will be the location where any necessary files are stored. Any process that wants to interact with the domain (including the casual CLI) must have this variable set to the same path as the domain itself. For most of this tutorial, we're going to assume that your domain is located in `casual/documentation/tutorials/operations/domain`. So, if you cloned this tutorials repo to your home directory, the value of your `CASUAL_DOMAIN_HOME` should be `$HOME/casual/documentation/tutorials/operations/domain`.

Let's put this into practice and set the `CASUAL_DOMAIN_HOME` variable; for example, if this repo is in your home directory:

```bash
$ export CASUAL_DOMAIN_HOME=$HOME/casual/documentation/tutorials/operations/domain
```

Confirm that the variable is correct:

```bash
$ echo $CASUAL_DOMAIN_HOME
/users/bob/casual/documentation/tutorials/operations/domain # or wherever you decided to put it
```

Until further notice, all instructions assume that your working directory is `casual/documentation/tutorials/operations/domain`:

```bash
$ cd $CASUAL_DOMAIN_HOME
```

Great! You're all set to continue to the next step.

### 2.3 Configuration

Most of everything you'll want your domain to do is specified through the configuration. This is where you tell casual which servers you want it to start, if you want queues, whether to connect to other domains, and much more. casual accepts configuration files written in YAML, JSON, XML, or INI, although we'll stick to YAML for this tutorial. Configuration files are either passed to the domain on startup, or applied afterwards in order to change an already running domain. You can start a domain without any configuration files, but that wouldn't be very interesting!

The first thing we're going to do is give our domain a name. This can be whatever suits you, but for the purposes of this tutorial it might be good to pick something unique, like `<your-name>-domain`. Open up `configuration/domain.yaml` and fill in your desired domain name, for example:

```yaml
domain:
  name: bobs-domain
```

We will return to this configuration file throughout this tutorial, but this is will do for starting a very basic domain.

## 3 Introducing the casual CLI

Before we start our first domain we're going to take a brief detour into the casual CLI. This is your primary tool for interacting with casual and offers a convenient way for controlling pretty much every aspect of your domain.

The CLI is divided into several different categories that covers the functionality related to that component. For example, `casual queue` holds all the options related to queues, `casual transaction` everything related to transactions, and so forth. At every level you can use the `--help` option to see all the options that are available along with a brief explanation. The CLI also offers convenient tab-completion (currently only for bash); try typing `casual s`and hitting tab for instance! Finally, some commands can be chained together by piping the output of one command into the next, more on that later. 

## 4 Getting started with servers

### 4.1 Our first domain

It's finally time to start our first domain! Simply pass the configuration file to `casual domain --boot`:

```bash
$ casual domain --boot configuration/domain.yaml
information: configuration files used: ["configuration/domain.yaml"]
task: boot domain  - started
...
```

Congratulations! You are now the proud owner of one, rather empty, casual domain. If you get an error from `casual domain --boot`, make sure that your domain.yaml is correctly formatted, YAML is very particular about indentation.

By running the command `casual domain --information`, you can see that your configuration has been applied:

```bash
$ casual domain --information
key                                             value
----------------------------------------------  --------------------------------
version.casual                                  1.6.16
version.compiler                                g++: 9.3.1
domain.identity.name                            bobs-domain
...
```

Take a moment to look around. You can use `casual service --list-services` to list all the services available in the domain and `casual queue --list-queues` to list all the queues. As you can see there's not a lot going on in our domain, let's change that.

We start by shutting the domain down:

```bash
$ casual domain --shutdown
task: shutdown domain - started
...
```

### 4.2 Adding servers

Typically, a casual domain will have one or more servers, each advertising one or more services that perform some task when called upon. Users can build their own servers for casual to manage, but we will be using the example-server that comes prebuilt with the casual installation.

When you already have a server, all you have to do is add it to the configuration, along with any additional options such as how many instances you want casual to start. Add the following to your domain.yaml, below `name`:

```yaml
domain:
  # ...
  servers:
      # this alias can be used when managing the server through the casual CLI
    - alias: my-example-server
      # the path to the executable, note the use of environment variables
      path: ${CASUAL_HOME}/example/bin/casual-example-server
      # servers can be scaled horizontally by adding instances 
      instances: 1
```

The full domain.yaml should now look something like this (the comments from above omitted for readability):

```yaml
domain:
  name: bobs-domain

  servers:
    - alias: my-example-server
      path: ${CASUAL_HOME}/example/bin/casual-example-server
      instances: 1
```

Boot up the domain:

```bash
$ casual domain --boot configuration/domain.yaml
information: configuration files used: ["configuration/domain.yaml"]
task: boot domain  - started
...
```

casual keeps track of all servers running in the domain, you can list them with `casual domain --list-servers`:

```bash
$ casual domain --list-servers
alias                       CI  I  restart  #r  path
--------------------------  --  -  -------  --  ----------------------------------------------------------------------
casual-domain-discovery      1  1     true   0  "/opt/casual/bin/casual-domain-discovery"
casual-domain-manager        1  1    false   0  "casual-domain-manager"
casual-gateway-manager       1  1     true   0  "/opt/casual/bin/casual-gateway-manager"
casual-queue-manager         1  1     true   0  "/opt/casual/bin/casual-queue-manager"
casual-service-manager       1  1     true   0  "/opt/casual/bin/casual-service-manager"
casual-transaction-manager   1  1     true   0  "/opt/casual/bin/casual-transaction-manager"
my-example-server            1  1    false   0  "${CASUAL_HOME}/example/bin/casual-example-server"
```

Do not be intimidated by all the abbreviations in the header, you can obtain a legend for this option by calling `casual domain --legend list-servers`. This feature is available for most options that output information.

As you can see, in addition to the server we told casual to start, there are a number of internal servers that expose various administration services. These are always present. Our server will automatically advertise its services, making them available to the domain. Check them out by calling `casual service --list-services`:

```bash
$ casual service --list-services
name                                                         category  V  mode    timeout  contract  I  C  AT     min    max    P  PAT    RI  RC  last
-----------------------------------------------------------  --------  -  ------  -------  --------  -  -  -----  -----  -----  -  -----  --  --  ----
casual/example/advertised/echo                               example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/atomic/echo                                   example   D  atomic        -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/auto/echo                                     example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation                                  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation_recv_send                        example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation_recv_send_auto                   example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/echo/78aa33c494134cbbb11b155a9b212c35  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/echo/bobs-domain                       example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/name                                   example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/echo                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/error/system                                  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward                                       example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward/echo                                  example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward/join/echo                             example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/lowercase                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/rollback                                      example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/sink                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/sleep                                         example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/terminate                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/undiscoverable/echo                           example   U  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/uppercase                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/work                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
```

Remember: you can use `casual service --legend list-services` to explain the columns of the output.

#### 4.2.1 Calling services from the CLI

In most cases, services are called programmatically by other services or executables, but the casual CLI has the ability to call services from the command line. Try running the following command:

```bash
$ cat configuration/domain.yaml | casual buffer --compose | casual call --service casual/example/echo | casual buffer --extract
domain:
  name: bobs-domain

  servers:
...
```

Let's dissect what we just did: first we piped the contents of our domain.yaml file to `casual buffer --compose`. This takes the input and creates a buffer in a casual-compatible format. Note that most services will have a more specific API, but `casual/example/echo` just returns whatever we give it. It is also worth mentioning that the slashes in the service name is just a naming convention, it could just as well have been called `casual_example_echo` or `steve` as far as casual is concerned.

Next, we pass our buffer to `casual call --service` with `casual/example/echo` as the argument specifying which service to call (try changing it to `casual/example/uppercase` for a different result). This performs the actual service call and returns the reply in the form of another buffer. Finally, the reply is passed to `casual buffer --extract` which extracts the data from the buffer and passes it to stdout. 

#### 4.2.2 Scaling servers

casual has the ability to start additional instances of servers when more capacity is needed. This can either be done through the configuration (see above), or through the CLI via the `casual domain --scale-aliases` option. Servers are referred to through their alias; to start another instance of our server, execute the following command:

```bash
$ casual domain --scale-aliases my-example-server 2
task: scale aliases - started
  alias spawn: my-example-server [17294]
  sub task:  - done
task: scale aliases - done
```

List the servers again to see the change:

```bash
$ casual domain --list-servers
alias                       CI  I  restart  #r  path
--------------------------  --  -  -------  --  ----------------------------------------------------------------------
casual-domain-discovery      1  1     true   0  "/opt/casual/bin/casual-domain-discovery"
casual-domain-manager        1  1    false   0  "casual-domain-manager"
casual-gateway-manager       1  1     true   0  "/opt/casual/bin/casual-gateway-manager"
casual-queue-manager         1  1     true   0  "/opt/casual/bin/casual-queue-manager"
casual-service-manager       1  1     true   0  "/opt/casual/bin/casual-service-manager"
casual-transaction-manager   1  1     true   0  "/opt/casual/bin/casual-transaction-manager"
my-example-server            2  2    false   0  "${CASUAL_HOME}/example/bin/casual-example-server"
```

That concludes our introduction to services, let's shut down our domain before we move on to queues:

```bash
$ casual domain --shutdown
task: shutdown domain - started
...
```

## 5 Queues and forwards

### 5.1 Queues

casual provides its own queue component where messages can be persistently stored and retrieved with transaction semantics. Servers and executables can interact with queues through a native API, though that is outside the scope of this tutorial.

Creating queues is as easy as adding a few lines of config. Append the following to your domain.yaml:

```yaml
domain:
  # ...
  queue:
    # each queue group is a single process and may have [0..n] queues 
    groups:
      # like services, aliases are used to refer to queue groups
      - alias: my-queue-group
        # these are the individual queues of the group
        queues:
          - name: a
          - name: b
          - name: c
```

When finished, your domain.yaml may look something like this (mind the indentation!):

```yaml
domain:
  name: bobs-domain

  servers:
    - alias: my-example-server
      path: ${CASUAL_HOME}/example/bin/casual-example-server
      instances: 1

  queue:
    groups:
      - alias: my-queue-group
        queues:
          - name: a
          - name: b
          - name: c
```

Start the domain anew:

```bash
$ casual domain --boot configuration/domain.yaml
information: configuration files used: ["configuration/domain.yaml"]
task: boot domain  - started
...
```

Take a look at our brand-new queues by calling `casual queue --list-queues`:

```bash
$ casual queue --list-queues
name     group           rc  rd     count  size  avg  EQ  DQ  UC  last
-------  --------------  --  -----  -----  ----  ---  --  --  --  ----
a        my-queue-group   0  0.000      0     0    0   0   0   0  -
b        my-queue-group   0  0.000      0     0    0   0   0   0  -
c        my-queue-group   0  0.000      0     0    0   0   0   0  -
a.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
b.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
c.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
```

As you can see, casual has created corresponding error queues for all the queues we configured. This is where messages end up after a (configurable) number of failed dequeues from the "non-error" ditto.

Unsurprisingly, all our queues are empty. Let's work some CLI magic to enqueue a message:

```bash
$ echo "hello world" | casual buffer --compose | casual queue --enqueue a
7864a6f4ebf9472ca0992987b143935e
```

The hex-string returned by the command is the id that casual has generated for the message. This id can be used when dequeuing to retrieve this particular message, rather than the oldest one on the queue.

If we list the queues again we can see that our message has been placed on the queue:

```bash
$ casual queue --list-queues
name     group           rc  rd     count  size  avg  EQ  DQ  UC  last
-------  --------------  --  -----  -----  ----  ---  --  --  --  --------------------------------
a        my-queue-group   0  0.000      1    12   12   1   0   0  2023-12-04T11:14:53.474623+01:00
b        my-queue-group   0  0.000      0     0    0   0   0   0  -
c        my-queue-group   0  0.000      0     0    0   0   0   0  -
a.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
b.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
c.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
```

We can also list all the messages of an individual queue:

```bash
$ casual queue --list-messages a
id                                S  size  trid  rd  type      reply  available  timestamp
--------------------------------  -  ----  ----  --  --------  -----  ---------  --------------------------------
7864a6f4ebf9472ca0992987b143935e  C    12         0  X_OCTET/                 -  2023-12-04T11:14:53.474623+01:00
```

Just as you can use the CLI to put a message on the queue, you can use it to take one off it:

```bash
$ casual queue --dequeue a | casual buffer --extract
hello world
```

Note that if there were multiple messages on the queue, and we wanted to dequeue this specific one, we could have provided the id after the queue name, like this: `casual queue --dequeue a 7864a6f4ebf9472ca0992987b143935e`.

The composability of the casual CLI means that you can create commands to perform more complex tasks, for example calling a service and putting the reply onto a queue:

```bash
$ echo "this is me shouting" | casual buffer --compose | casual call --service casual/example/uppercase | casual queue --enqueue b
d0a92263c6534c8e8da41339125bc011
$ casual queue --dequeue b | casual buffer --extract
THIS IS ME SHOUTING
```

There are many more options related to queues available in the casual CLI, feel free to have a look around before shutting the domain down in preparation for the next section:

```bash
$ casual domain --shutdown
task: shutdown domain - started
...
```

#### 5.2 Forwards

While casual provides an API for executables to programmatically enqueue and dequeue messages, sometimes what is needed is an entity that simply continuously dequeues messages from a queue and either enqueues them onto another queue or uses their contents to call a service. To this end, casual offers two flavours of *forward*: queues and services, that does just that.

Let's return to our domain.yaml once more to add some forwards under queue:

```yaml
domain:
  # ...
  queue:
    # ...
    forward:
      # like queues, a forward group is a single process but may consist of multiple forwards
      groups:
        - alias: my-forward-group
          # a group may have both forward queues and services
          queues:
            # this forward will dequeue from the queue 'a' and enqueue those messages onto 'b'
            - alias: my-forward-queue
              instances: 1
              source: a
              target:
                queue: b
          services:
            # service forwards are slightly more complex as the reply from the service call may then be put onto another queue
            # this forward will dequeue from 'b', call casual/example/uppercase, and put the reply onto 'c'
            - alias: my-forward-service
              instances: 1
              source: b
              target:
                service: casual/example/uppercase
              reply:
                queue: c
```

After these latest additions, your full domain.yaml should be similar to the following:

```yaml
domain:
  name: bobs-domain

  servers:
    - alias: my-example-server
      path: ${CASUAL_HOME}/example/bin/casual-example-server
      instances: 1

  queue:
    groups:
      - alias: my-queue-group
        queues:
          - name: a
          - name: b
          - name: c

    forward:
      groups:
        - alias: my-forward-group
          queues:
            - alias: my-forward-queue
              instances: 1
              source: a
              target:
                queue: b
          services:
            - alias: my-forward-service
              instances: 1
              source: b
              target:
                service: casual/example/uppercase
              reply:
                queue: c
```

Pay extra close attention to the indentation here: since `forward` is part of the queue configuration, it needs to be indented to the same level as the above `groups`.

Well done! Go ahead and boot up the domain:

```bash
$ casual domain --boot configuration/domain.yaml
information: configuration files used: ["configuration/domain.yaml"]
task: boot domain  - started
...
```

Our new forward group should be up and running:

```bash
$ casual queue --list-forward-groups
alias             pid   services  queues  commits  rollbacks  last
----------------  ----  --------  ------  -------  ---------  ----
my-forward-group  1024         1       1        0          0  -

```

We can also look at the individual forwards:

```bash
$ casual queue --list-forward-queues
alias             group             source  target  delay  CI  I  commits  rollbacks  last
----------------  ----------------  ------  ------  -----  --  -  -------  ---------  ----
my-forward-queue  my-forward-group  a       b       0.000  1   1        0          0  -

$ casual queue --list-forward-services
alias               group             source  target                    reply  delay     CI  I  commits  rollbacks  last
------------------  ----------------  ------  ------------------------  -----  --------  --  -  -------  ---------  ----
my-forward-service  my-forward-group  b       casual/example/uppercase  c      0.000000  1   1        0          0  -
```

As you might have already noticed, the setup we have configured means that any message enqueued to the queue `a` will first be forwarded to `b` by the queue forward. From there the service forward will dequeue it, call `casual/example/uppercase` and put the reply on `c`. Let's try it out:

```bash
$ echo "some nonsense" | casual buffer --compose | casual queue --enqueue a
bca5b9c290c947ac81f242a9602baf69
```

We can expect to find a message on `c`:

```bash
$ casual queue --list-queues
name     group           rc  rd     count  size  avg  EQ  DQ  UC  last
-------  --------------  --  -----  -----  ----  ---  --  --  --  --------------------------------
a        my-queue-group   0  0.000      0     0    0   2   2   0  2023-12-04T16:23:05.747907+01:00
b        my-queue-group   0  0.000      0     0    0   2   2   0  2023-12-04T16:23:05.784388+01:00
c        my-queue-group   0  0.000      1    15   15   1   0   0  2023-12-04T16:23:05.797320+01:00
a.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
b.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
c.error  my-queue-group   0  0.000      0     0    0   0   0   0  -
```

Let's have a look at it:

```bash
$ casual queue --dequeue c | casual buffer --extract
SOME NONSENSE
```

Much like servers, forwards have a configurable number of instances running at any one time. They can be scaled up or down by referring to their aliases:

```bash
$ casual queue --forward-scale-aliases my-forward-service 0
```

We now have zero running instances of the forward:

```bash
$ casual queue --list-forward-services
alias               group             source  target                    reply  delay     CI  I  commits  rollbacks  last
------------------  ----------------  ------  ------------------------  -----  --------  --  -  -------  ---------  --------------------------------
my-forward-service  my-forward-group  b       casual/example/uppercase  c      0.000000  0   0        1          0  2023-12-04T16:23:05.799746+01:00
```

Note that the forwards will try to complete any ongoing calls before scaling down, so it might take a while before they reach the configured number of instances.

That's it for queues and forwards, let's shut down our domain before we move on:

```bash
$ casual domain --shutdown
task: shutdown domain - started
...
```

## 6 Gateways

### 6.1 Setup

For this next exercise we're going to expand our universe beyond the confines of a single domain. casual has the ability to connect to other domains, allowing you access to resources such as queues and services in the remote domain. This makes it possible to create large distributed systems with components that can be deployed independently of each other.

For this lab however, we'll settle for connecting to a single remote domain. We've taken the liberty of preparing one for you; it's hiding under `remote` in the root directory of this tutorial, all that remains for you to do is start a new terminal, make sure the [prerequisites](#21-prerequisites) are in place, and do the following:

```bash
# From a new terminal session:
$ cd <path-to-the-casual-operations-tutorial-directory>/remote
$ export CASUAL_DOMAIN_HOME=`pwd`
$ casual domain --boot configuration/domain.yaml
```

Have a look at the services and queues available in this domain:

```bash
$ casual service --list-services
name            category  V  mode  timeout  contract  I  C  AT     min    max    P  PAT    RI  RC  last
--------------  --------  -  ----  -------  --------  -  -  -----  -----  -----  -  -----  --  --  ----
remote-service  example   D  join        -    linger  1  0  0.000  0.000  0.000  0  0.000   0   0  -

$ casual queue --list-queues
name                group               rc  rd     count  size  avg  EQ  DQ  UC  last
------------------  ------------------  --  -----  -----  ----  ---  --  --  --  ----
remote-queue        remote-queue-group   0  0.000      0     0    0   0   0   0  -
remote-queue.error  remote-queue-group   0  0.000      0     0    0   0   0   0  -
```

As you can see, this domain has its own queues and services that do not exist in our domain. But by establishing a connection to this remote domain, these resources will become available within our domain.

Switch back to your previous terminal, but leave the terminal with the remote domain running for the remainder of the exercise.

### 6.2 Configuring gateway groups

The remote domain has been configured with an *inbound group*, a component that listens for incoming connections, and when one is established, exposes its local resources to incoming requests.

The inbound configuration in the remote domain looks like this (we're using port 7778 for this exercise, if that port is taken, feel free to change it, but remember to edit both the inbound and outbound groups accordingly):

```yaml
domain:
  # ...
  gateway:
    inbound:
      # A domain may have multiple inbound (and outbound) groups
      groups:
        - alias: remote-domain-inbound
          note: listens for incoming connections
          # An inbound group may listen on multiple endpoints
          connections:
            # The ip and port combination on which to listen. The corresponding outbound group connects to this endpoint.
            - address: "127.0.0.1:7778"
```

The other side of this pair is the *outbound group*, a component that connects to an inbound group and acts as a client towards the remote domain when a local process wants to access remote resources. Since that is precisely what we want to do, we're going to start an outbound group in our domain by adding the following config:

```yaml
domain:
  # ...
  gateway:
    outbound:
      groups:
        - alias: my-outbound
          note: connects to remote-domain
          # An outbound group may connect to multiple inbound groups 
          connections:
            # The ip and port combination to connect to, corresponds to the inbound group in the remote domain.
            - address: "127.0.0.1:7778"
```

Our ever-expanding domain.yaml should now contain the following:

```yaml
domain:
  name: bobs-domain

  servers:
    - alias: my-example-server
      path: ${CASUAL_HOME}/example/bin/casual-example-server
      instances: 1

  queue:
    groups:
      - alias: my-queue-group
        queues:
          - name: a
          - name: b
          - name: c

    forward:
      groups:
        - alias: my-forward-group
          queues:
            - alias: my-forward-queue
              instances: 1
              source: a
              target:
                queue: b
          services:
            - alias: my-forward-service
              instances: 1
              source: b
              target:
                service: casual/example/uppercase
              reply:
                queue: c

  gateway:
    outbound:
      groups:
        - alias: my-outbound
          note: connects to remote-domain
          connections:
            - address: "127.0.0.1:7778"
```

With no further ado, let's start the domain back up:

```bash
$ casual domain --boot configuration/domain.yaml
information: configuration files used: ["configuration/domain.yaml"]
task: boot domain  - started
...
```

Our new outbound group will establish a connection to the other domain. Use the CLI to take a look:

```bash
$ casual gateway --list-connections
name           id                                group        bound  runlevel   local            peer            created
-------------  --------------------------------  -----------  -----  ---------  ---------------  --------------  --------------------------------
remote-domain  c88befb7ac3c4d8a876b37606dadafc0  my-outbound  out    connected  127.0.0.1:35928  127.0.0.1:7778  2024-01-02T11:25:46.831204+01:00
```

However, if we look at the available services...

```bash
$ casual service --list-services
name                                                         category  V  mode    timeout  contract  I  C  AT     min    max    P  PAT    RI  RC  last
-----------------------------------------------------------  --------  -  ------  -------  --------  -  -  -----  -----  -----  -  -----  --  --  ----
casual/example/advertised/echo                               example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/atomic/echo                                   example   D  atomic        -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/auto/echo                                     example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation                                  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation_recv_send                        example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/conversation_recv_send_auto                   example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/echo/bb7819224f8c4a9c8467db5fcccf8e89  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/echo/bobs-domain                       example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/domain/name                                   example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/echo                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/error/system                                  example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward                                       example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward/echo                                  example   D  auto          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/forward/join/echo                             example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/lowercase                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/rollback                                      example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/sink                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/sleep                                         example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/terminate                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/undiscoverable/echo                           example   U  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/uppercase                                     example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
casual/example/work                                          example   D  join          -         -  1  0  0.000  0.000  0.000  0  0.000   0   0  -
```

...no new service has appeared, what gives?!

Don't panic: casual will only look for resources outside the local domain when they are requested. This *discovery* happens automatically, but it can also be triggered manually. Let's see if we can use the CLI to find the service we saw in the remote domain:

```bash
$ casual discovery --services remote-service
name            hops
--------------  ----
remote-service     1
```

There it is! The service will now be listed on subsequent uses of `--list-services`, but with 0 in the `I` (for instances) column, indicating that there are no local servers advertising it:

```bash
$ casual service --list-services
name                                                         category  V  mode    timeout  contract  I  C  AT     min    max    P  PAT    RI  RC  last
-----------------------------------------------------------  --------  -  ------  -------  --------  -  -  -----  -----  -----  -  -----  --  --  ----
...
remote-service                                               example   D  join          -         -  0  0  0.000  0.000  0.000  0  0.000   1   0  -
...
```

If we want to know where the service is actually "from", we can consult the gateway:

```bash
$ casual gateway --list-services
service         name           id                                peer
--------------  -------------  --------------------------------  --------------
remote-service  remote-domain  c88befb7ac3c4d8a876b37606dadafc0  127.0.0.1:7778
```

Here, `name` is the alias of the local outbound group that advertises the service, while `peer` is the actual connection (remember: an outbound group may have multiple connections) to the domain with the service.

Now, having the service show up in lists is all well and good, but can you actually call it? `remote-service` is a simple service that just replies with the name of the domain in which its server is running. Let's try calling it:

```bash
$ echo "hello remote domain" | casual buffer --compose | casual call --service remote-service | casual buffer --extract
remote-domain
```

And there you have it! From the caller's perspective it's just like calling any local service, no matter if the remote domain is running in different container or on the Moon.

Similarly, remote queues will be automatically discovered when requested and can be enqueued and dequeued like any other queue:

```bash
$ echo "an important message" | casual buffer --compose | casual queue --enqueue remote-queue
bd6d30ea41d0481d8c27907b1ba51683
$ casual queue --dequeue remote-queue | casual buffer --extract
an important message
```

However, unlike services, remote queues will not be listed when using `casual queue --list-queues`. We can however ask the gateway whether it knows of any remote queues:

```bash
$ casual gateway --list-queues
queue         name           id                                peer
------------  -------------  --------------------------------  --------------
remote-queue  remote-domain  c88befb7ac3c4d8a876b37606dadafc0  127.0.0.1:7778
```

It is important to note that this outbound-inbound pairing is one way only. Try to call any of the services in our domain from the remote domain and all you'll get is an error. Of course, this could be remedied by configuring another connection but with the inbound group in this domain and the outbound group in the remote (using a different port).

Anyway, let's shut everything down:

```bash
$ casual domain --shutdown
task: shutdown domain - started
...
```

And don't forget the terminal with the remote domain!

```bash
# from the other terminal
$ casual domain --shutdown
task: shutdown domain - started
...
```

### 6.3 Reverse gateway groups

A factor to consider when working with multiple domains is that the outbound group is responsible for establishing the connection with the inbound group. However, sometimes a situation arises where the address of the inbound is dynamic or otherwise not known ahead of time. For these situations casual offers a reverse pair of inbound and outbound groups where the inbound connects to the outbound (all other functionality remains the same as the regular pairing). To configure a reverse group, simply add `reverse` between `gateway` and the `inbound`/`outbound` objects, for example:

```yaml
domain:
  # ...
  gateway:
    reverse:
      inbound:
        groups:
          - alias: "my-reverse-inbound"
            connections:
              # Being a reverse inbound, the group will actively try to connect to this address, rather than passively listening for incoming connections. 
              - address: "127.0.0.1:7779"
```

That concludes this tutorial. While we have covered the basics of how to operate a domain, casual has a lot more features to offer. A good way to discover some of them is to use the `--help` flag of the CLI, or reading the documentation at https://casualcore.github.io/.
