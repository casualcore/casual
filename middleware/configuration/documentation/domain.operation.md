# configuration domain

This is the runtime configuration for a casual domain.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  

The following _sections_ can be used:

## transaction

Defines transaction related configuration.

### log

The path of the distributed transactions log file. When a distributed transaction reaches prepare,
this state is persistent stored, before the actual commit stage.

if `:memory:` is used, the log is non-persistent. 

### resources

Defines all resources that `servers` and `executables` can be associated with, within this configuration.

property       | description
---------------|----------------------------------------------------
key            | the resource key - correlates to a defined resource in the resource.property (for current _machine_)
name           | a user defined name, used to correlate this resource within the rest of the configuration. 
instances      | number of resource-proxy instances transaction-manger should start. To handle distributed transactions
openinfo       | resources specific _open_ configurations for the particular resource.
closeinfo      | resources specific _close_ configurations for the particular resource.


## groups

Defines the groups in the configuration. Groups are used to associate `resources` to serveres/executables
and to define the dependency order.

property       | description
---------------|----------------------------------------------------
name           | the name (unique key) of the group.
dependencies   | defines which other groups this group has dependency to.
resources      | defines which resources (name) this group associate with (transient to the members of the group)


## servers

Defines all servers of the configuration (and domain) 

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the server. If not provided basename of `path` will be used
arguments      | arguments to `tpsvrinit` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order, and possible resource association)
restrictions   | service restrictions, if provided the intersection of _restrictions_ and advertised services are actually advertised.
resources      | explicit resource associations (transaction.resources.name)


## executables

Defines all _ordinary_ executables of the configuration. Could be any executable with a `main` function

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the executable. If not provided basename of `path` will be used
arguments      | arguments to `main` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order)


## services

Defines service related configuration. 

Note that this configuration is tied to the service, regardless who has advertised the service.

property       | description
---------------|----------------------------------------------------
name           | name of the service
timeout        | timeout of the service, from the _caller_ perspective (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)
routes         | defines what logical names are actually exposed. For _aliases_, it's important to include the original name.


## gateway

Defines configuration for communication with other `casual` domains.

### listeners

Defines all listeners that this configuration should listen on.

property       | description
---------------|----------------------------------------------------
address        | the address to listen on, `host:port` 
limit.size     | the maximum allowed size of all inflight messages. If reached _inbound_ stop taking more request until below the limit 
limit.messages | the maximum allowed number of inflight messages. If reached _inbound_ stop taking more request until below the limit


### connections

Defines all _outbound_ connections that this configuration should try to connect to.

The order in which connections are defined matters. services and queues found in the first connection has higher priority
than the last, hence if several remote domains exposes the the same service, the first connection will be used as long as
the service is provided. When a connection is lost, the next connection that exposes the service will be used.


property       | description
---------------|----------------------------------------------------
address        | the address to connect to, `host:port` 
restart        | if true, the connection will be restarted if connection is lost.
services       | services we're expecting to find on the other side 
queues         | queues we're expecting to find on the other side 

`services` and `queues` is used as an _optimization_ to do a _build_ discovery during startup. `casual`
will find these services later lazily otherwise. It can also be used to do some rudimentary load balancing 
to make sure lower prioritized connections are used for `services` and `queues` that could be discovered in
higher prioritized connections.

## queue

Defines the queue configuration

### groups

Defines groups of queues which share the same storage location. Groups has no other meaning.

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the group.
queuebase      | the path to the storage file. (default: `${CASUAL_DOMAIN_HOM}/queue/<group-name>.qb`, if `:memory:` is used, the storage is non persistent)
queues         | defines all queues in this group, see below

#### groups.queues

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the queue.
retry.count    | number of rollbacks before moving message to `<queue-name>.error`.
retry.delay    | if message is rolled backed, how long delay before message is avaliable for dequeue. (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)



## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
domain:
  name: domain.A42
  default:
    environment:
      files:
        - /some/path/to/environment/file
        - /some/other/file

      variables:
        - key: SOME_VARIABLE
          value: 42

        - key: SOME_OTHER_VARIABLE
          value: some value



    server:
      instances: 1
      restart: true
      memberships:
        []

      environment:
        files:
          []

        variables:
          []



    executable:
      instances: 1
      restart: false
      memberships:
        []

      environment:
        files:
          []

        variables:
          []



    service:
      timeout: 90s


  transaction:
    default:
      resource:
        key: db2_rm
        instances: 3


    log: /some/fast/disk/domain.A42/transaction.log
    resources:
      - name: customer-db
        instances: 5
        note: this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5
        openinfo: db=customer,uid=db2,pwd=db2

      - name: sales-db
        note: this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances
        openinfo: db=sales,uid=db2,pwd=db2

      - name: event-queue
        key: mq_rm
        note: this resource is named 'event-queue' - overrides rm-key - using default rm-instances
        openinfo: some-mq-specific-stuff
        closeinfo: some-mq-specific-stuff



  groups:
    - name: common-group
      note: group that logically groups 'common' stuff

    - name: customer-group
      note: group that logically groups 'customer' stuff
      resources:
        - customer-db

      dependencies:
        - common-group


    - name: sales-group
      note: group that logically groups 'customer' stuff
      resources:
        - sales-db
        - event-queue

      dependencies:
        - customer-group



  servers:
    - path: customer-server-1
      memberships:
        - customer-group


    - path: customer-server-2
      memberships:
        - customer-group


    - path: sales-server
      alias: sales-pre
      note: the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'
      instances: 10
      memberships:
        - sales-group

      restrictions:
        - preSalesSaveService
        - preSalesGetService


    - path: sales-server
      alias: sales-post
      note: the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'
      memberships:
        - sales-group

      restrictions:
        - postSalesSaveService
        - postSalesGetService


    - path: sales-broker
      memberships:
        - sales-group

      environment:
        files:
          []

        variables:
          - key: SALES_BROKER_VARIABLE
            value: 556



      resources:
        - event-queue



  executables:
    - path: mq-server
      arguments:
        - --configuration
        - /path/to/configuration

      memberships:
        - common-group



  services:
    - name: postSalesSaveService
      timeout: 2h
      routes:
        - postSalesSaveService
        - sales/post/save


    - name: postSalesGetService
      timeout: 130ms


  gateway:
    default:
      listener:
        limit:
          {}


      connection:
        restart: true
        address: ""


    listeners:
      - address: localhost:7779
        limit:
          size: 2097152

        note: local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below

      - address: some.host.org:7779
        limit:
          messages: 200

        note: listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'

      - address: some.host.org:9999
        limit:
          size: 10485760
          messages: 10

        note: listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume

      - address: some.host.org:4242
        note: listener - no limits


    connections:
      - address: a45.domain.host.org:7779
        services:
          - s1
          - s2

        queues:
          []

        note: connection to domain 'a45' - we expect to find service 's1' and 's2' there.

      - address: a46.domain.host.org:7779
        services:
          - s1

        queues:
          - q1
          - q2

        note: connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)

      - address: a99.domain.host.org:7780
        services:
          []

        queues:
          []

        restart: false
        note: connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection



  queue:
    default:
      queue:
        retry:
          count: 3
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




    note: "retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\x0a"


...

````
### json
```` json
{
    "domain": {
        "name": "domain.A42",
        "default": {
            "environment": {
                "files": [
                    "/some/path/to/environment/file",
                    "/some/other/file"
                ],
                "variables": [
                    {
                        "key": "SOME_VARIABLE",
                        "value": "42"
                    },
                    {
                        "key": "SOME_OTHER_VARIABLE",
                        "value": "some value"
                    }
                ]
            },
            "server": {
                "instances": 1,
                "restart": true,
                "memberships": [],
                "environment": {
                    "files": [],
                    "variables": []
                }
            },
            "executable": {
                "instances": 1,
                "restart": false,
                "memberships": [],
                "environment": {
                    "files": [],
                    "variables": []
                }
            },
            "service": {
                "timeout": "90s"
            }
        },
        "transaction": {
            "default": {
                "resource": {
                    "key": "db2_rm",
                    "instances": 3
                }
            },
            "log": "/some/fast/disk/domain.A42/transaction.log",
            "resources": [
                {
                    "name": "customer-db",
                    "instances": 5,
                    "note": "this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5",
                    "openinfo": "db=customer,uid=db2,pwd=db2"
                },
                {
                    "name": "sales-db",
                    "note": "this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances",
                    "openinfo": "db=sales,uid=db2,pwd=db2"
                },
                {
                    "name": "event-queue",
                    "key": "mq_rm",
                    "note": "this resource is named 'event-queue' - overrides rm-key - using default rm-instances",
                    "openinfo": "some-mq-specific-stuff",
                    "closeinfo": "some-mq-specific-stuff"
                }
            ]
        },
        "groups": [
            {
                "name": "common-group",
                "note": "group that logically groups 'common' stuff"
            },
            {
                "name": "customer-group",
                "note": "group that logically groups 'customer' stuff",
                "resources": [
                    "customer-db"
                ],
                "dependencies": [
                    "common-group"
                ]
            },
            {
                "name": "sales-group",
                "note": "group that logically groups 'customer' stuff",
                "resources": [
                    "sales-db",
                    "event-queue"
                ],
                "dependencies": [
                    "customer-group"
                ]
            }
        ],
        "servers": [
            {
                "path": "customer-server-1",
                "memberships": [
                    "customer-group"
                ]
            },
            {
                "path": "customer-server-2",
                "memberships": [
                    "customer-group"
                ]
            },
            {
                "path": "sales-server",
                "alias": "sales-pre",
                "note": "the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'",
                "instances": 10,
                "memberships": [
                    "sales-group"
                ],
                "restrictions": [
                    "preSalesSaveService",
                    "preSalesGetService"
                ]
            },
            {
                "path": "sales-server",
                "alias": "sales-post",
                "note": "the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'",
                "memberships": [
                    "sales-group"
                ],
                "restrictions": [
                    "postSalesSaveService",
                    "postSalesGetService"
                ]
            },
            {
                "path": "sales-broker",
                "memberships": [
                    "sales-group"
                ],
                "environment": {
                    "files": [],
                    "variables": [
                        {
                            "key": "SALES_BROKER_VARIABLE",
                            "value": "556"
                        }
                    ]
                },
                "resources": [
                    "event-queue"
                ]
            }
        ],
        "executables": [
            {
                "path": "mq-server",
                "arguments": [
                    "--configuration",
                    "/path/to/configuration"
                ],
                "memberships": [
                    "common-group"
                ]
            }
        ],
        "services": [
            {
                "name": "postSalesSaveService",
                "timeout": "2h",
                "routes": [
                    "postSalesSaveService",
                    "sales/post/save"
                ]
            },
            {
                "name": "postSalesGetService",
                "timeout": "130ms"
            }
        ],
        "gateway": {
            "default": {
                "listener": {
                    "limit": {}
                },
                "connection": {
                    "restart": true,
                    "address": ""
                }
            },
            "listeners": [
                {
                    "address": "localhost:7779",
                    "limit": {
                        "size": 2097152
                    },
                    "note": "local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below"
                },
                {
                    "address": "some.host.org:7779",
                    "limit": {
                        "messages": 200
                    },
                    "note": "listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'"
                },
                {
                    "address": "some.host.org:9999",
                    "limit": {
                        "size": 10485760,
                        "messages": 10
                    },
                    "note": "listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume"
                },
                {
                    "address": "some.host.org:4242",
                    "note": "listener - no limits"
                }
            ],
            "connections": [
                {
                    "address": "a45.domain.host.org:7779",
                    "services": [
                        "s1",
                        "s2"
                    ],
                    "queues": [],
                    "note": "connection to domain 'a45' - we expect to find service 's1' and 's2' there."
                },
                {
                    "address": "a46.domain.host.org:7779",
                    "services": [
                        "s1"
                    ],
                    "queues": [
                        "q1",
                        "q2"
                    ],
                    "note": "connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)"
                },
                {
                    "address": "a99.domain.host.org:7780",
                    "services": [],
                    "queues": [],
                    "restart": false,
                    "note": "connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection"
                }
            ]
        },
        "queue": {
            "default": {
                "queue": {
                    "retry": {
                        "count": 3,
                        "delay": "20s"
                    }
                },
                "directory": "${CASUAL_DOMAIN_HOME}/queue/groups"
            },
            "groups": [
                {
                    "name": "groupA",
                    "note": "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb",
                    "queues": [
                        {
                            "name": "q_A1"
                        },
                        {
                            "name": "q_A2",
                            "retry": {
                                "count": 10,
                                "delay": "100ms"
                            },
                            "note": "after 10 rollbacked dequeues, message is moved to q_A2.error"
                        },
                        {
                            "name": "q_A3"
                        },
                        {
                            "name": "q_A4"
                        }
                    ]
                },
                {
                    "name": "groupB",
                    "queuebase": "/some/fast/disk/queue/groupB.qb",
                    "queues": [
                        {
                            "name": "q_B1"
                        },
                        {
                            "name": "q_B2",
                            "retry": {
                                "count": 20
                            },
                            "note": "after 20 rollbacked dequeues, message is moved to q_B2.error. retry.delay is 'inherited' from default, if any"
                        }
                    ]
                },
                {
                    "name": "groupC",
                    "queuebase": ":memory:",
                    "note": "group is an in-memory queue, hence no persistence",
                    "queues": [
                        {
                            "name": "q_C1"
                        },
                        {
                            "name": "q_C2"
                        }
                    ]
                }
            ],
            "note": "retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\n"
        }
    }
}
````
### ini
```` ini

[domain]
name=domain.A42

[domain.default]

[domain.default.environment]
files=/some/path/to/environment/file
files=/some/other/file

[domain.default.environment.variables]
key=SOME_VARIABLE
value=42

[domain.default.environment.variables]
key=SOME_OTHER_VARIABLE
value=some value

[domain.default.executable]
instances=1
restart=false

[domain.default.executable.environment]

[domain.default.server]
instances=1
restart=true

[domain.default.server.environment]

[domain.default.service]
timeout=90s

[domain.executables]
arguments=--configuration
arguments=/path/to/configuration
memberships=common-group
path=mq-server

[domain.gateway]

[domain.gateway.connections]
address=a45.domain.host.org:7779
note=connection to domain 'a45' - we expect to find service 's1' and 's2' there.
services=s1
services=s2

[domain.gateway.connections]
address=a46.domain.host.org:7779
note=connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)
queues=q1
queues=q2
services=s1

[domain.gateway.connections]
address=a99.domain.host.org:7780
note=connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection
restart=false

[domain.gateway.default]

[domain.gateway.default.connection]
address=
restart=true

[domain.gateway.default.listener]

[domain.gateway.default.listener.limit]

[domain.gateway.listeners]
address=localhost:7779
note=local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below

[domain.gateway.listeners.limit]
size=2097152

[domain.gateway.listeners]
address=some.host.org:7779
note=listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'

[domain.gateway.listeners.limit]
messages=200

[domain.gateway.listeners]
address=some.host.org:9999
note=listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume

[domain.gateway.listeners.limit]
messages=10
size=10485760

[domain.gateway.listeners]
address=some.host.org:4242
note=listener - no limits

[domain.groups]
name=common-group
note=group that logically groups 'common' stuff

[domain.groups]
dependencies=common-group
name=customer-group
note=group that logically groups 'customer' stuff
resources=customer-db

[domain.groups]
dependencies=customer-group
name=sales-group
note=group that logically groups 'customer' stuff
resources=sales-db
resources=event-queue

[domain.queue]
note=retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\n

[domain.queue.default]
directory=${CASUAL_DOMAIN_HOME}/queue/groups

[domain.queue.default.queue]

[domain.queue.default.queue.retry]
count=3
delay=20s

[domain.queue.groups]
name=groupA
note=will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb

[domain.queue.groups.queues]
name=q_A1

[domain.queue.groups.queues]
name=q_A2
note=after 10 rollbacked dequeues, message is moved to q_A2.error

[domain.queue.groups.queues.retry]
count=10
delay=100ms

[domain.queue.groups.queues]
name=q_A3

[domain.queue.groups.queues]
name=q_A4

[domain.queue.groups]
name=groupB
queuebase=/some/fast/disk/queue/groupB.qb

[domain.queue.groups.queues]
name=q_B1

[domain.queue.groups.queues]
name=q_B2
note=after 20 rollbacked dequeues, message is moved to q_B2.error. retry.delay is 'inherited' from default, if any

[domain.queue.groups.queues.retry]
count=20

[domain.queue.groups]
name=groupC
note=group is an in-memory queue, hence no persistence
queuebase=:memory:

[domain.queue.groups.queues]
name=q_C1

[domain.queue.groups.queues]
name=q_C2

[domain.servers]
memberships=customer-group
path=customer-server-1

[domain.servers]
memberships=customer-group
path=customer-server-2

[domain.servers]
alias=sales-pre
instances=10
memberships=sales-group
note=the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'
path=sales-server
restrictions=preSalesSaveService
restrictions=preSalesGetService

[domain.servers]
alias=sales-post
memberships=sales-group
note=the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'
path=sales-server
restrictions=postSalesSaveService
restrictions=postSalesGetService

[domain.servers]
memberships=sales-group
path=sales-broker
resources=event-queue

[domain.servers.environment]

[domain.servers.environment.variables]
key=SALES_BROKER_VARIABLE
value=556

[domain.services]
name=postSalesSaveService
routes=postSalesSaveService
routes=sales/post/save
timeout=2h

[domain.services]
name=postSalesGetService
timeout=130ms

[domain.transaction]
log=/some/fast/disk/domain.A42/transaction.log

[domain.transaction.default]

[domain.transaction.default.resource]
instances=3
key=db2_rm

[domain.transaction.resources]
instances=5
name=customer-db
note=this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5
openinfo=db=customer,uid=db2,pwd=db2

[domain.transaction.resources]
name=sales-db
note=this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances
openinfo=db=sales,uid=db2,pwd=db2

[domain.transaction.resources]
closeinfo=some-mq-specific-stuff
key=mq_rm
name=event-queue
note=this resource is named 'event-queue' - overrides rm-key - using default rm-instances
openinfo=some-mq-specific-stuff

````
### xml
```` xml
<?xml version="1.0"?>
<domain>
 <name>domain.A42</name>
 <default>
  <environment>
   <files>
    <element>/some/path/to/environment/file</element>
    <element>/some/other/file</element>
   </files>
   <variables>
    <element>
     <key>SOME_VARIABLE</key>
     <value>42</value>
    </element>
    <element>
     <key>SOME_OTHER_VARIABLE</key>
     <value>some value</value>
    </element>
   </variables>
  </environment>
  <server>
   <instances>1</instances>
   <restart>true</restart>
   <memberships />
   <environment>
    <files />
    <variables />
   </environment>
  </server>
  <executable>
   <instances>1</instances>
   <restart>false</restart>
   <memberships />
   <environment>
    <files />
    <variables />
   </environment>
  </executable>
  <service>
   <timeout>90s</timeout>
  </service>
 </default>
 <transaction>
  <default>
   <resource>
    <key>db2_rm</key>
    <instances>3</instances>
   </resource>
  </default>
  <log>/some/fast/disk/domain.A42/transaction.log</log>
  <resources>
   <element>
    <name>customer-db</name>
    <instances>5</instances>
    <note>this resource is named 'customer-db' - using the default rm-key (db_rm) - overrides the default rm-instances to 5</note>
    <openinfo>db=customer,uid=db2,pwd=db2</openinfo>
   </element>
   <element>
    <name>sales-db</name>
    <note>this resource is named 'sales-db' - using the default rm-key (db_rm) - using default rm-instances</note>
    <openinfo>db=sales,uid=db2,pwd=db2</openinfo>
   </element>
   <element>
    <name>event-queue</name>
    <key>mq_rm</key>
    <note>this resource is named 'event-queue' - overrides rm-key - using default rm-instances</note>
    <openinfo>some-mq-specific-stuff</openinfo>
    <closeinfo>some-mq-specific-stuff</closeinfo>
   </element>
  </resources>
 </transaction>
 <groups>
  <element>
   <name>common-group</name>
   <note>group that logically groups 'common' stuff</note>
  </element>
  <element>
   <name>customer-group</name>
   <note>group that logically groups 'customer' stuff</note>
   <resources>
    <element>customer-db</element>
   </resources>
   <dependencies>
    <element>common-group</element>
   </dependencies>
  </element>
  <element>
   <name>sales-group</name>
   <note>group that logically groups 'customer' stuff</note>
   <resources>
    <element>sales-db</element>
    <element>event-queue</element>
   </resources>
   <dependencies>
    <element>customer-group</element>
   </dependencies>
  </element>
 </groups>
 <servers>
  <element>
   <path>customer-server-1</path>
   <memberships>
    <element>customer-group</element>
   </memberships>
  </element>
  <element>
   <path>customer-server-2</path>
   <memberships>
    <element>customer-group</element>
   </memberships>
  </element>
  <element>
   <path>sales-server</path>
   <alias>sales-pre</alias>
   <note>the only services that will be advertised are 'preSalesSaveService' and 'preSalesGetService'</note>
   <instances>10</instances>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <restrictions>
    <element>preSalesSaveService</element>
    <element>preSalesGetService</element>
   </restrictions>
  </element>
  <element>
   <path>sales-server</path>
   <alias>sales-post</alias>
   <note>the only services that will be advertised are 'postSalesSaveService' and 'postSalesGetService'</note>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <restrictions>
    <element>postSalesSaveService</element>
    <element>postSalesGetService</element>
   </restrictions>
  </element>
  <element>
   <path>sales-broker</path>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <environment>
    <files />
    <variables>
     <element>
      <key>SALES_BROKER_VARIABLE</key>
      <value>556</value>
     </element>
    </variables>
   </environment>
   <resources>
    <element>event-queue</element>
   </resources>
  </element>
 </servers>
 <executables>
  <element>
   <path>mq-server</path>
   <arguments>
    <element>--configuration</element>
    <element>/path/to/configuration</element>
   </arguments>
   <memberships>
    <element>common-group</element>
   </memberships>
  </element>
 </executables>
 <services>
  <element>
   <name>postSalesSaveService</name>
   <timeout>2h</timeout>
   <routes>
    <element>postSalesSaveService</element>
    <element>sales/post/save</element>
   </routes>
  </element>
  <element>
   <name>postSalesGetService</name>
   <timeout>130ms</timeout>
  </element>
 </services>
 <gateway>
  <default>
   <listener>
    <limit />
   </listener>
   <connection>
    <restart>true</restart>
    <address></address>
   </connection>
  </default>
  <listeners>
   <element>
    <address>localhost:7779</address>
    <limit>
     <size>2097152</size>
    </limit>
    <note>local host - if threshold of 2MB of total payload 'in flight' is reach inbound will stop consume from socket until we're below</note>
   </element>
   <element>
    <address>some.host.org:7779</address>
    <limit>
     <messages>200</messages>
    </limit>
    <note>listener that is bound to some 'external ip' - limit to max 200 calls 'in flight'</note>
   </element>
   <element>
    <address>some.host.org:9999</address>
    <limit>
     <size>10485760</size>
     <messages>10</messages>
    </limit>
    <note>listener - threshold of either 10 messages OR 10MB - the first that is reach, inbound will stop consume</note>
   </element>
   <element>
    <address>some.host.org:4242</address>
    <note>listener - no limits</note>
   </element>
  </listeners>
  <connections>
   <element>
    <address>a45.domain.host.org:7779</address>
    <services>
     <element>s1</element>
     <element>s2</element>
    </services>
    <queues />
    <note>connection to domain 'a45' - we expect to find service 's1' and 's2' there.</note>
   </element>
   <element>
    <address>a46.domain.host.org:7779</address>
    <services>
     <element>s1</element>
    </services>
    <queues>
     <element>q1</element>
     <element>q2</element>
    </queues>
    <note>connection to domain 'a46' - we expect to find queues 'q1' and 'q2' and service 's1' - casual will only route 's1' to a46 if it's not accessible in a45 (or local)</note>
   </element>
   <element>
    <address>a99.domain.host.org:7780</address>
    <services />
    <queues />
    <restart>false</restart>
    <note>connection to domain 'a99' - if the connection is closed from a99, casual will not try to reestablish the connection</note>
   </element>
  </connections>
 </gateway>
 <queue>
  <default>
   <queue>
    <retry>
     <count>3</count>
     <delay>20s</delay>
    </retry>
   </queue>
   <directory>${CASUAL_DOMAIN_HOME}/queue/groups</directory>
  </default>
  <groups>
   <element>
    <name>groupA</name>
    <note>will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb</note>
    <queues>
     <element>
      <name>q_A1</name>
     </element>
     <element>
      <name>q_A2</name>
      <retry>
       <count>10</count>
       <delay>100ms</delay>
      </retry>
      <note>after 10 rollbacked dequeues, message is moved to q_A2.error</note>
     </element>
     <element>
      <name>q_A3</name>
     </element>
     <element>
      <name>q_A4</name>
     </element>
    </queues>
   </element>
   <element>
    <name>groupB</name>
    <queuebase>/some/fast/disk/queue/groupB.qb</queuebase>
    <queues>
     <element>
      <name>q_B1</name>
     </element>
     <element>
      <name>q_B2</name>
      <retry>
       <count>20</count>
      </retry>
      <note>after 20 rollbacked dequeues, message is moved to q_B2.error. retry.delay is 'inherited' from default, if any</note>
     </element>
    </queues>
   </element>
   <element>
    <name>groupC</name>
    <queuebase>:memory:</queuebase>
    <note>group is an in-memory queue, hence no persistence</note>
    <queues>
     <element>
      <name>q_C1</name>
     </element>
     <element>
      <name>q_C2</name>
     </element>
    </queues>
   </element>
  </groups>
  <note>retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback
</note>
 </queue>
</domain>

````
