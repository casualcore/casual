# casual.broker


## .casual.broker.state

Gets the current state of the broker for a given domain

### definition

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

### example

```json
{
    "input": {},
    "output": {
        "serviceReturn": {
            "instances": [
                {
                    "process": {
                        "pid": 297,
                        "queue": 663
                    },
                    "state": "a",
                    "invoked": 758,
                    "last": 922
                }
            ],
            "services": [
                {
                    "name": "casual",
                    "timeout": 0,
                    "instances": [
                        156
                    ],
                    "lookedup": 355,
                    "type": 829,
                    "transaction": 126
                }
            ],
            "pending": [
                {
                    "requested": "casual",
                    "process": {
                        "pid": 196,
                        "queue": 329
                    }
                }
            ]
        }
    }
}

```