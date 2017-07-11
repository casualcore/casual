# casual.domain

## .casual.domain.state

### Definition

```yaml
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

### Example

```json
{
    "input": {},
    "output": {
        "serviceReturn": {
            "groups": [
                {
                    "id": 380,
                    "name": "casual",
                    "note": "casual",
                    "resources": [
                        {
                            "instances": 562,
                            "key": "casual",
                            "openinfo": "casual",
                            "closeinfo": "casual"
                        }
                    ],
                    "dependencies": [
                        771
                    ]
                }
            ],
            "executables": [
                {
                    "id": 623,
                    "alias": "casual",
                    "path": "casual",
                    "arguments": [
                        "casual"
                    ],
                    "note": "casual",
                    "instances": [
                        287
                    ],
                    "memberships": [
                        632
                    ],
                    "environment": {
                        "variables": [
                            "casual"
                        ]
                    },
                    "configured_instances": 120,
                    "restart": true,
                    "restarts": 884
                }
            ],
            "termination": {
                "listeners": [
                    {
                        "pid": 503,
                        "queue": 58
                    }
                ]
            }
        }
    }
}

```

## .casual.domain.scale.instances

### Definition

```yaml
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

### Example

```json
{
    "input": {
        "instances": [
            {
                "alias": "casual",
                "instances": 256
            }
        ]
    },
    "output": {
        "serviceReturn": [
            {
                "alias": "casual",
                "instances": 838
            }
        ]
    }
}
```

## .casual.domain.shutdown

### Definition

```yaml
service: .casual.domain.shutdown
input
output
```

### Example
```json
{
    "input": {},
    "output": {}
}
```
