
## API definitions of all the administration services casual have

### Describe services
To describe a casaul-sf-service just use the following command:

```bash
>$ casual-service-describe --services <services>
```


### Services
The following services is described:

* .casual.broker.state
* .casual.domain.scale.instances
* .casual.domain.shutdown  
* .casual.domain.state
* .casual.transaction.state
* .casual.transaction.update.instances


#### .casual.broker.state

```bash
service: .casual.broker.state
input
output
  composite serviceReturn
    container instances
      composite
        composite process
          integer pid
          integer queue
        character state
        integer invoked
        integer last
    container services
      composite
        string name
        integer timeout
        container instances
          integer
        integer lookedup
        integer type
        integer transaction
    container pending
      composite
        string requested
        composite process
          integer pid
          integer queue
```

#### .casual.domain.scale.instances

```bash
service: .casual.domain.scale.instances
input
  container instances
    composite
      string alias
      integer instances
output
  container serviceReturn
    composite
      string alias
      integer instances
```

#### .casual.domain.shutdown

```bash
service: .casual.domain.shutdown
input
output
```

#### .casual.domain.state

```bash

service: .casual.domain.state
input
output
  composite serviceReturn
    container groups
      composite
        integer id
        string name
        string note
        container resources
          composite
            integer instances
            string key
            string openinfo
            string closeinfo
        container dependencies
          integer
    container executables
      composite
        integer id
        string alias
        string path
        container arguments
          string
        string note
        container instances
          integer
        container memberships
          integer
        composite environment
          container variables
            string
        integer configured_instances
        boolean restart
        integer restarts
    composite termination
      container listeners
        composite
          integer pid
          integer queue


```

#### .casual.transaction.state

```bash
service: .casual.transaction.state
input
output
  composite serviceReturn
    container resources
      composite
        integer id
        string key
        string openinfo
        string closeinfo
        integer concurency
        composite statistics
          composite resource
            integer min
            integer max
            integer total
            integer invoked
          composite roundtrip
            integer min
            integer max
            integer total
            integer invoked
        container instances
          composite
            integer id
            composite process
              integer pid
              integer queue
            integer state
            composite statistics
              composite resource
                integer min
                integer max
                integer total
                integer invoked
              composite roundtrip
                integer min
                integer max
                integer total
                integer invoked
    container transactions
      composite
        composite trid
          integer type
          composite owner
            integer pid
            integer queue
          string global
          string branch
        container resources
          integer
        integer state
    container persistent.replies
      composite
        integer queue
        string correlation
        integer type
    container persistent.requests
      composite
        container resources
          integer
        string correlation
        integer type
    container pending.requests
      composite
        container resources
          integer
        string correlation
        integer type
    composite log
      composite update
        integer prepare
        integer remove
      integer writes

```

#### .casual.transaction.update.instances

```bash
service: .casual.transaction.update.instances
input
  container instances
    composite
      integer id
      integer instances
output
  container serviceReturn
    composite
      integer id
      string key
      string openinfo
      string closeinfo
      integer concurency
      composite statistics
        composite resource
          integer min
          integer max
          integer total
          integer invoked
        composite roundtrip
          integer min
          integer max
          integer total
          integer invoked
      container instances
        composite
          integer id
          composite process
            integer pid
            integer queue
          integer state
          composite statistics
            composite resource
              integer min
              integer max
              integer total
              integer invoked
            composite roundtrip
              integer min
              integer max
              integer total
              integer invoked


```
