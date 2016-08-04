# casual.gateway


## .casual.gateway.state

Gets the current state of the gateway for a given domain

### definition

```bash
service: .casual.gateway.state
input
output
  composite serviceReturn
    composite connections
      container outbound
        composite 
          composite process
            integer pid
            integer queue
          composite remote
            string name
            binary id
          container address
            string 
          character type
          integer runlevel
      container inbound
        composite 
          composite process
            integer pid
            integer queue
          composite remote
            string name
            binary id
          container address
            string 
          character type
          integer runlevel
```

### example

```json
{
    "input": {},
    "output": {
        "serviceReturn": {
            "connections": {
                "outbound": [
                    {
                        "process": {
                            "pid": 85,
                            "queue": 138
                        },
                        "remote": {
                            "name": "casual",
                            "id": "iftuCSsoQIetzctNQfWuGg=="
                        },
                        "address": [
                            "casual"
                        ],
                        "type": "4",
                        "runlevel": 487
                    }
                ],
                "inbound": [
                    {
                        "process": {
                            "pid": 975,
                            "queue": 171
                        },
                        "remote": {
                            "name": "casual",
                            "id": "Zm3EUHU/TqqBGDhfsFUc2Q=="
                        },
                        "address": [
                            "casual"
                        ],
                        "type": "(",
                        "runlevel": 984
                    }
                ]
            }
        }
    }
}

```