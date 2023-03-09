
# casual-transaction-manager

## .casual.transaction.state

Gets the current state of the transaction manager in a given domain

### definition

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

## .casual.transaction.update.instances

Updates instances of 0..* resource proxies

### definition
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
