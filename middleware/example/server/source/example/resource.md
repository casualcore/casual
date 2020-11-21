# casual-example-resource-server

## configuration 

When using `casual-example-resource-server` you need to configure the _domain configuration_ 
so `casual-example-resource-server` gets the configuration it needs.

`casual-example-resource-server` is compiled and linked with a _resource name_, hence `casual`
expect to find the same _configuration name_ in the configuration (from `casual-domain-manager`).

The _resource_name_ is, and has to be, `example-resource-server`

### example domain configuration

``` yaml 
domain:
  name: some-domain
  transaction:
    resources:
      - name: example-resource-server # the name that `casual-example-resource-server` will lookup to get configuration
        key: rm-mockup  # key to lookup the _resource-proxy-server_
        openinfo: "foo" # does not matter in this case (it's a mockup...)
        instances: 1

  servers:
    - path: ${CASUAL_HOME}/example/bin/casual-example-resource-server
      instances: 2 
      # ... no resource "configured" since `casual-example-resource-server` look up with the name _example-resource-server_
      # the name could be anything (locally unique) but in this case we've chose _example-resource-server_

```

## precondition

`casual` needs to have _configuration_ for the _resource-proxy-server_. 

TODO: link to _CASUAL_RESOURCE_CONFIGURATION_FILE_ ?


