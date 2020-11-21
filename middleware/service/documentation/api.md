# casual-service-manager


## .casual/service/state

Gets the current state of the broker for a given domain

### definition

```bash
service: .casual/service/state
input
output
  composite result
    composite instances
      container local
        composite 
          composite process
            integer pid
            integer queue
          character state
      container remote
        composite 
          composite process
            integer pid
            integer queue
    container services
      composite 
        string name
        integer timeout
        string category
        integer transaction
        composite metrics
          integer count
          integer total
        composite pending
          integer count
          integer total
        integer remote_invocations
        integer last
        composite instances
          container local
            composite 
              integer pid
          container remote
            composite 
              integer pid
              integer hops
    container pending
      composite 
        string requested
        composite process
          integer pid
          integer queue

```

## .casual/service/metric/reset

Resets the metric for provided services.

Resets all services if none are provided.

### definition

```bash
service: .casual/service/metric/reset
input
  container services
    string 
output
  container result
    string 
```
