

## .casual.domain.state
<a name=".casual.domain.state"></a>

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

## .casual.domain.scale.instances
<a name=".casual.domain.scale.instances"></a>

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

## .casual.domain.shutdown
<a name=".casual.domain.shutdown"></a>

```bash
service: .casual.domain.shutdown
input
output
```
