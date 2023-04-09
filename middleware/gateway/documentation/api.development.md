# casual-gateway-manager


## .casual/gateway/state

Gets the current state of the gateway for a given domain

### definition

```bash
service: .casual/gateway/state
input
output
  composite result
    container connections
      composite 
        character bound
        character type
        integer runlevel
        composite process
          integer pid
          integer queue
        composite remote
          string name
          binary id
        container address
          string
```

